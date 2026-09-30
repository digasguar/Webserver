#ifndef CGIRUNNER_HPP
# define CGIRUNNER_HPP

#include <map>
#include "Client.hpp"
#include "ConfigTypes.hpp"
#include "CgiProcess.hpp"

bool tryStartCgiForClient(int clientFd, unsigned long clientSerial, const HttpRequesr &request, const std::string &filePath,
                          const LocationConfig &loc, int epoll_fd,
                          std::map<int, CgiProcess> &cgiByReadFd,
                          std::map<int, int> &writeFdToReadFd);

void handleCgiWrite(std::map<int, Client> &clients, std::map<int, CgiProcess> &cgiByReadFd,
                     std::map<int, int> &writeFdToReadFd, int write_fd, int epoll_fd);

void handleCgiRead(std::map<int, Client> &clients, std::map<int, CgiProcess> &cgiByReadFd,
                    std::map<int, int> &writeFdToReadFd, int read_fd, int epoll_fd);

#endif
