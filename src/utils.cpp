#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"
#include <vector>

int calculate_index(int current_fd, const std::map<int, const ServerConfig*> &listenFds, epoll_event ep)
{
    if (listenFds.find(current_fd) != listenFds.end())
        return (1);
    if (ep.events & (EPOLLERR | EPOLLHUP))
        return (4);
    if (ep.events & EPOLLIN)
        return (2);
    if (ep.events & EPOLLOUT)
        return (3);
    return (0);
}

void close_conection(std::map<int, Client> &clients, int current_fd, int epoll_fd)
{
    std::map<int, Client>::iterator it = clients.find(current_fd);
    if (it != clients.end() && it->second.getFileFd() != -1)
        close(it->second.getFileFd());
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
    close(current_fd);
    if (it != clients.end())
        clients.erase(it);
}

void finishResponse(std::map<int, Client> &clients, int current_fd, int epoll_fd)
{
    std::map<int , Client>::iterator it = clients.find(current_fd);
    if (it == clients.end())
        return ;
    Client& client = clients.at(current_fd);

    if (client.getFileFd() != -1)
        close(client.getFileFd());
    if (!client.getKeepAlive())
    {
        close_conection(clients, current_fd, epoll_fd);
        return ;
    }
    client.resetRequest();
    epoll_event ev;
    ev.data.fd = current_fd;
    ev.events = EPOLLIN;
    client.setEpollEvent(ev);
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &ev) == -1)
        close_conection(clients, current_fd, epoll_fd);
}

void checkClientTimeut(std::map<int, Client> &clients, int epoll_fd)
{
    static time_t lastCheck = 0;
    time_t now = time(NULL);
    if (now == lastCheck)
        return ;
    lastCheck = now;

    std::vector<int> expired;
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (now - it->second.getLastActivity() >= CLIENT_TIMEOUT)
            expired.push_back(it->first);
    }
    for (size_t i = 0; i < expired.size(); ++i)
    {
        std::cout << "Client timeout: " << expired[i] << std::endl;
        close_conection(clients, expired[i], epoll_fd);
    }
}

std::string statusMessage(const std::string &status)
{
    static const struct { const char *code; const char *message; } table [] =
    {
        // 1xx - Informational
        { "100", "Continue" },
        { "101", "Switching Protocols" },
        { "102", "Processing" },
        { "103", "Early Hints" },

        // 2xx - Success
        { "200", "OK" },
        { "201", "Created" },
        { "202", "Accepted" },
        { "203", "Non-Authoritative Information" },
        { "204", "No Content" },
        { "205", "Reset Content" },
        { "206", "Partial Content" },
        { "207", "Multi-Status" },
        { "208", "Already Reported" },
        { "226", "IM Used" },

        // 3xx - Redirection
        { "300", "Multiple Choices" },
        { "301", "Moved Permanently" },
        { "302", "Found" },
        { "303", "See Other" },
        { "304", "Not Modified" },
        { "305", "Use Proxy" }, //deprecado no se usa.
        { "306", "Switch Proxy" },//deprecado no se usa.
        { "307", "Temporary Redirect" },
        { "308", "Permanent Redirect" },

        // 4xx - Client Error
        { "400", "Bad Request" },
        { "401", "Unauthorized" },
        { "402", "Payment Required" },
        { "403", "Forbidden" },
        { "404", "Not Found" },
        { "405", "Method Not Allowed" },
        { "406", "Not Acceptable" },
        { "407", "Proxy Authentication Required" },
        { "408", "Request Timeout" },
        { "409", "Conflict" },
        { "410", "Gone" },
        { "411", "Length Required" },
        { "412", "Precondition Failed" },
        { "413", "Payload Too Large" },
        { "414", "URI Too Long" },
        { "415", "Unsupported Media Type" },
        { "416", "Range Not Satisfiable" },
        { "417", "Expectation Failed" },
        { "418", "I'm a Teapot" },
        { "421", "Misdirected Request" },
        { "422", "Unprocessable Entity" },
        { "423", "Locked" },
        { "424", "Failed Dependency" },
        { "425", "Too Early" },
        { "426", "Upgrade Required" },
        { "428", "Precondition Required" },
        { "429", "Too Many Requests" },
        { "431", "Request Header Fields Too Large" },
        { "451", "Unavailable For Legal Reasons" },

        // 5xx - Server Error
        { "500", "Internal Server Error" },
        { "501", "Not Implemented" },
        { "502", "Bad Gateway" },
        { "503", "Service Unavailable" },
        { "504", "Gateway Timeout" },
        { "505", "HTTP Version Not Supported" },
        { "506", "Variant Also Negotiates" },
        { "507", "Insufficient Storage" },
        { "508", "Loop Detected" },
        { "510", "Not Extended" },
        { "511", "Network Authentication Required" }
    };
    size_t tableSize = sizeof(table) / sizeof(table[0]);
    for (size_t i = 0; i < tableSize; ++i)
    {
        if (status == table[i].code)
            return (table[i].message);
    }
    return ("Unknown Status Code");
}