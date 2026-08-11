#include "../includes/Librari.hpp"

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