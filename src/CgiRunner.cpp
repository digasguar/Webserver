#include "../includes/CgiRunner.hpp"
#include "../includes/CgiEnv.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>
#include <cctype>
#include <signal.h>

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
	proc.startTime = time(NULL);

    epoll_event readEv;
    readEv.data.fd = proc.readFd;
    readEv.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, proc.readFd, &readEv);

    if (request.body.empty())
    {
		closeTracked(proc.writeFd);
    }
    else
    {
		epoll_event writeEv;
		writeEv.data.fd = proc.writeFd;
		writeEv.events = EPOLLOUT;
		epoll_ctl(epoll_fd, EPOLL_CTL_ADD, proc.writeFd, &writeEv);
		writeFdToReadFd[proc.writeFd] = proc.readFd;
    }

	g_touchedFds.insert(proc.readFd);
	g_touchedFds.insert(proc.writeFd);

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
		closeTracked(write_fd);
		writeFdToReadFd.erase(wIt);
		return;
    }

    const std::string &body = clIt->second.getRequest().body;

    if (proc.bodyBytesSent >= body.size())
    {
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, write_fd, NULL);
		closeTracked(write_fd);
		writeFdToReadFd.erase(wIt);
		return;
    }

    ssize_t sent = write(write_fd, body.c_str() + proc.bodyBytesSent, body.size() - proc.bodyBytesSent);
    if (sent <= 0)
    {
		epoll_ctl(epoll_fd, EPOLL_CTL_DEL, write_fd, NULL);
		closeTracked(write_fd);
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
    if (n < 0)
		return; // no es EOF (por ejempol lectura sin datos) asi que esperamos al siguiente evento

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, read_fd, NULL);
    closeTracked(read_fd);

	int status = 0;
    waitpid(proc.pid, &status, 0);
    
    bool crashed = (!WIFEXITED(status) || WEXITSTATUS(status) != 0);

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

		std::string statusLine = "200 OK";
		std::string keptHeaders;
		{   // extrae "Status: NNN msg" de las cabeceras del CGI y lo usa como linea de estado
		    size_t i = 0;
		    while (i < cgiHeaders.size())
		    {
				size_t e = cgiHeaders.find('\n', i);
				if (e == std::string::npos) e = cgiHeaders.size();
				std::string line = cgiHeaders.substr(i, e - i);
				if (!line.empty() && line[line.size() - 1] == '\r') line.erase(line.size() - 1);
				std::string low = line;
				for (size_t k = 0; k < low.size(); ++k) low[k] = std::tolower(static_cast<unsigned char>(low[k]));
				if (low.compare(0, 7, "status:") == 0)
				{
				    size_t v = line.find_first_not_of(" \t", 7);
				    if (v != std::string::npos) statusLine = line.substr(v);
				}
				else if (!line.empty() && low.compare(0, 15, "content-length:") != 0) // el servidor pone su propio Content-Length
				    keptHeaders += line + "\r\n";
				i = e + 1;
		    }
		}
		if (crashed && proc.outputSoFar.empty())
		{
		    statusLine = "502 Bad Gateway";
		    cgiBody = "502 Bad Gateway: the CGI script failed";
		    keptHeaders = "Content-Type: text/plain\r\n";
		}


		std::stringstream response;
		response << "HTTP/1.1 " << statusLine << "\r\n";
		response << keptHeaders;

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
		    closeTracked(wIt->first);
		    writeFdToReadFd.erase(wIt);
		    break;
		}
    }

    cgiByReadFd.erase(cIt);
}

// mata los CGI cuyo cliente ya no existe (cerro la conexion) o que tardan mas de CGI_TIMEOUT.
// si el cliente sigue ahi, le responde 504. Se llama en cada vuelta del bucle principal.
void reapDeadOrSlowCgi(std::map<int, Client> &clients, std::map<int, CgiProcess> &cgiByReadFd,
                       std::map<int, int> &writeFdToReadFd, int epoll_fd)
{
    time_t now = time(NULL);
    std::vector<int> victims;
    for (std::map<int, CgiProcess>::iterator it = cgiByReadFd.begin(); it != cgiByReadFd.end(); ++it)
    {
        std::map<int, Client>::iterator c = clients.find(it->second.clientFd);
        bool clientGone = (c == clients.end() || c->second.getSerial() != it->second.clientSerial);
        if (clientGone || now - it->second.startTime >= CGI_TIMEOUT)
            victims.push_back(it->first);
    }
    for (size_t i = 0; i < victims.size(); ++i)
    {
        int rfd = victims[i];
        std::map<int, CgiProcess>::iterator it = cgiByReadFd.find(rfd);
        if (it == cgiByReadFd.end())
            continue;
        CgiProcess proc = it->second;
        kill(proc.pid, SIGKILL);
        waitpid(proc.pid, NULL, 0);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, rfd, NULL);
        closeTracked(rfd);
        for (std::map<int, int>::iterator w = writeFdToReadFd.begin(); w != writeFdToReadFd.end(); ++w)
        {
            if (w->second == rfd)
            {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, w->first, NULL);
                closeTracked(w->first);
                writeFdToReadFd.erase(w);
                break;
            }
        }
        cgiByReadFd.erase(it);

        std::map<int, Client>::iterator c = clients.find(proc.clientFd);
        if (c != clients.end() && c->second.getSerial() == proc.clientSerial)
        {
            std::string body = "504 Gateway Timeout";
            std::stringstream h;
            h << "HTTP/1.1 504 Gateway Timeout\r\nContent-Type: text/plain\r\nContent-Length: " << body.size()
              << "\r\nConnection: close\r\n\r\n";
            Client &client = c->second;
            client.setKeepAlive(false);
            client.setResponseHeaders(h.str());
            client.setBuffer(body.c_str(), body.size());
            client.setFileOffset(0);
            client.setIsRegularFile(true);
            client.setFileSize(body.size());
            client.setFileFd(-1);
            epoll_event ev;
            ev.data.fd = proc.clientFd;
            ev.events = EPOLLOUT;
            client.setEpollEvent(ev);
            epoll_ctl(epoll_fd, EPOLL_CTL_MOD, proc.clientFd, &ev);
        }
    }
}

void killAllCgi(std::map<int, CgiProcess> &cgiByReadFd, std::map<int, int> &writeFdToReadFd)
{
    for (std::map<int, CgiProcess>::iterator it = cgiByReadFd.begin(); it != cgiByReadFd.end(); ++it)
    {
		kill(it->second.pid, SIGKILL);
		waitpid(it->second.pid, NULL, 0); // tras SIGKILL no bloquea; evita zombies
		closeTracked(it->first);
    }
    for (std::map<int, int>::iterator it = writeFdToReadFd.begin(); it != writeFdToReadFd.end(); ++it)
		closeTracked(it->first);
    cgiByReadFd.clear();
    writeFdToReadFd.clear();
}
