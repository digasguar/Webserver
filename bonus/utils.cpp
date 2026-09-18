#include "includes/Librari.hpp"
#include "includes/Client.hpp"
#include "includes/CookiesManager.hpp"

int calculate_index(int current_fd, int fd, epoll_event ep)
{
    if (current_fd == fd)
        return (1);
    if (ep.events & EPOLLIN)
        return (2);
    if (ep.events & EPOLLOUT)
        return (3);
    return (0);
}

void close_conection(std::map<int, Client> &clients, int current_fd, int epoll_fd)
{
    close(current_fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
    clients.erase(current_fd);
    std::cout << "FIN\n";
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
    time_t now = time(NULL);
    std::map<int, Client>::iterator it = clients.begin();

    while(it != clients.end())
    {
        if (now - it->second.getLastActivity() >= CLIENT_TIMEOUT)
        {
            std::cout << "Client timeout: " <<  it->first << std::endl;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, NULL);
            close(it->first);
            std::map<int, Client>::iterator toErrase = it;
            it++;
            clients.erase(toErrase);
        }
        else
            it++;
    }
}

std::string extractCookie(const std::string &cookieHeader)
{
    std::string prefix = "session_id=";
    size_t pos = cookieHeader.find(prefix);

    if (pos == std::string::npos)
        return ("");
    return (cookieHeader.substr(pos + prefix.size()));
}

bool isPublicRoute(const std::string &path)
{
    std::string publicRoutes[] = {"/login", "/login/submit"};
    size_t routeCount = sizeof(publicRoutes) / sizeof(publicRoutes[0]);
    for(size_t i = 0; i < routeCount; i++)
    {
        if (path == publicRoutes[i])
            return (true);
    }
    return (false);
}

std::string extractFormField(const std::string &body, const std::string &field)
{
    std::string searchName = field + "=";
    size_t pos = body.find(searchName);

    if (pos == std::string::npos)
        return ("");
    pos += searchName.size();

    size_t end = body.find('&', pos);
    std::string value;

    if (end == std::string::npos)
        value = body.substr(pos);
    else
        value = body.substr(pos, end - pos);
    return (value); //No se si tendriamos que usar percentDecode mejor aqui o no pero bueno eso ya a criterio de asier 
}

void requestRedirectToLogin(Client *client)
{
    std::string headers = createAuthRedirect("/login", client->getKeepAlive());

    client->setResponseHeaders(headers);
    client->setBuffer("", 0);
    client->setFileOffset(0);
    client->setIsRegularFile(true);
    client->setFileSize(0);
}

void requestLoginPage(Client *client)
{
    std::string headers = createHeadersLength("text/html", "200 OK", LOGIN_PAGE.size(), client->getKeepAlive());

    client->setResponseHeaders(headers);
    client->setBuffer(LOGIN_PAGE.c_str(), LOGIN_PAGE.size());
    client->setFileOffset(0);
    client->setIsRegularFile(true);
    client->setFileSize(LOGIN_PAGE.size());
}

void requestLoginSubmit(Client *client, CookiesManager &cookieManager)
{
    std::string username = extractFormField(client->getRequest().body, "username");

    if (username.empty())
    {
        std::string headers = createAuthRedirect("/login", client->getKeepAlive());
        client->setResponseHeaders(headers);
        client->setBuffer("", 0);
        client->setFileOffset(0);
        client->setIsRegularFile(true);
        client->setFileSize(0);
        return;
    }

    std::string hash = cookieManager.createCookie(username);
    std::string cookieValue = "session_id=" + hash;
    std::string headers = createAuthRedirectWithCookie("/", cookieValue, client->getKeepAlive());

    client->setResponseHeaders(headers);
    client->setBuffer("", 0);
    client->setFileOffset(0);
    client->setIsRegularFile(true);
    client->setFileSize(0);
}