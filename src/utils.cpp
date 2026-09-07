#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"

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