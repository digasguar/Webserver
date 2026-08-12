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
    clients.erase(current_fd);
    close(current_fd);
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
    std::cout << "FIN\n";
}

void finishResponse(std::map<int, Client> &clients, int current_fd, int epoll_fd)
{
    Client& client = clients.at(current_fd);
    if (!client.getKeepAlive())
    {
        
        close_conection(clients, current_fd, epoll_fd);
        return ;
    }
    else
        client.resetRequest();
}