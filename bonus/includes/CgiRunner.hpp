#ifndef CGIRUNNER_HPP
# define CGIRUNNER_HPP

#define CGI_TIMEOUT 30 // segundos maximos que puede tardar un CGI antes de matarlo y responder 504

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
                    
void reapDeadOrSlowCgi(std::map<int, Client> &clients, std::map<int, CgiProcess> &cgiByReadFd,
					std::map<int, int> &writeFdToReadFd, int epoll_fd);

void killAllCgi(std::map<int, CgiProcess> &cgiByReadFd, std::map<int, int> &writeFdToReadFd);

#endif
