#include "../includes/CgiRunner.hpp"
#include "../includes/CgiEnv.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>

static std::string extensionOf(const std::string &path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return ("");
    return (path.substr(dot));
}

bool tryStartCgiForClient(int clientFd, unsigned long clientSerial, const HttpRequesr &request, const std::string &filePath,
                          const LocationConfig &loc, int epoll_fd,
                          std::map<int, CgiProcess> &cgiByReadFd,
                          std::map<int, int> &writeFdToReadFd)
{
    std::string ext = extensionOf(filePath);
    std::map<std::string, std::string>::const_iterator it = loc.cgiHandlers.find(ext);
    if (it == loc.cgiHandlers.end())
        return (false);

    CgiProcess proc;
    if (!startCgi(request, filePath, it->second, proc))
        return (false); // TODO: esto deberia devolver un 500, no caer al flujo normal

    proc.clientFd = clientFd;
    proc.clientSerial = clientSerial;

    epoll_event readEv;
    readEv.data.fd = proc.readFd;
    readEv.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, proc.readFd, &readEv);

    if (request.body.empty())
    {
        close(proc.writeFd);
    }
    else
    {
        epoll_event writeEv;
        writeEv.data.fd = proc.writeFd;
        writeEv.events = EPOLLOUT;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, proc.writeFd, &writeEv);
        writeFdToReadFd[proc.writeFd] = proc.readFd;
    }

    cgiByReadFd[proc.readFd] = proc;
    return (true);
}

void handleCgiWrite(std::map<int, Client> &clients, std::map<int, CgiProcess> &cgiByReadFd,
                     std::map<int, int> &writeFdToReadFd, int write_fd, int epoll_fd)
{
    std::map<int, int>::iterator wIt = writeFdToReadFd.find(write_fd);
    if (wIt == writeFdToReadFd.end())
        return;
    int read_fd = wIt->second;

    std::map<int, CgiProcess>::iterator cIt = cgiByReadFd.find(read_fd);
    if (cIt == cgiByReadFd.end())
        return;
    CgiProcess &proc = cIt->second;

    std::map<int, Client>::iterator clIt = clients.find(proc.clientFd);
    if (clIt == clients.end() || clIt->second.getSerial() != proc.clientSerial)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, write_fd, NULL);
        close(write_fd);
        writeFdToReadFd.erase(wIt);
        return;
    }

    const std::string &body = clIt->second.getRequest().body;

    if (proc.bodyBytesSent >= body.size())
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, write_fd, NULL);
        close(write_fd);
        writeFdToReadFd.erase(wIt);
        return;
    }

    ssize_t sent = write(write_fd, body.c_str() + proc.bodyBytesSent, body.size() - proc.bodyBytesSent);
    if (sent <= 0)
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, write_fd, NULL);
        close(write_fd);
        writeFdToReadFd.erase(wIt);
        return;
    }
    proc.bodyBytesSent += sent;
}

void handleCgiRead(std::map<int, Client> &clients, std::map<int, CgiProcess> &cgiByReadFd,
                    std::map<int, int> &writeFdToReadFd, int read_fd, int epoll_fd)
{
    std::map<int, CgiProcess>::iterator cIt = cgiByReadFd.find(read_fd);
    if (cIt == cgiByReadFd.end())
        return;
    CgiProcess &proc = cIt->second;

    char buf[4096];
    ssize_t n = read(read_fd, buf, sizeof(buf));

    if (n > 0)
    {
        proc.outputSoFar.append(buf, n);
        return;
    }

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, read_fd, NULL);
    close(read_fd);

    int status;
    waitpid(proc.pid, &status, 0);

    std::map<int, Client>::iterator clIt = clients.find(proc.clientFd);
    if (clIt != clients.end() && clIt->second.getSerial() == proc.clientSerial)
    {
        Client &client = clIt->second;

        std::string cgiOut = proc.outputSoFar;
        size_t headerEnd = cgiOut.find("\r\n\r\n");
        size_t sepLen = 4;
        size_t altEnd = cgiOut.find("\n\n");
        if (headerEnd == std::string::npos || (altEnd != std::string::npos && altEnd < headerEnd))
        {
            headerEnd = altEnd;
            sepLen = 2;
        }

        std::string cgiHeaders, cgiBody;
        if (headerEnd != std::string::npos)
        {
            cgiHeaders = cgiOut.substr(0, headerEnd);
            cgiBody = cgiOut.substr(headerEnd + sepLen);
        }
        else
            cgiBody = cgiOut;

        std::stringstream response;
        response << "HTTP/1.1 200 OK\r\n";
        if (!cgiHeaders.empty())
            response << cgiHeaders << "\r\n";
        response << "Content-Length: " << cgiBody.size() << "\r\n";
        response << (client.getKeepAlive() ? "Connection: keep-alive\r\n" : "Connection: close\r\n");
        response << "\r\n";

        client.setResponseHeaders(response.str());
        client.setBuffer(cgiBody.c_str(), cgiBody.size());
        client.setFileOffset(0);
        client.setIsRegularFile(true);
        client.setFileSize(cgiBody.size());
        client.setFileFd(-1);

        epoll_event response_event;
        response_event.data.fd = proc.clientFd;
        response_event.events = EPOLLOUT;
        client.setEpollEvent(response_event);
        epoll_ctl(epoll_fd, EPOLL_CTL_MOD, proc.clientFd, &response_event);
    }

    for (std::map<int, int>::iterator wIt = writeFdToReadFd.begin(); wIt != writeFdToReadFd.end(); ++wIt)
    {
        if (wIt->second == read_fd)
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, wIt->first, NULL);
            close(wIt->first);
            writeFdToReadFd.erase(wIt);
            break;
        }
    }

    cgiByReadFd.erase(cIt);
}
