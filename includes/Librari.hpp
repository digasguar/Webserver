#ifndef LIBRARI_H
# define LIBRARI_H
#pragma once

class Client;
struct ServerConfig;
struct CgiProcess;
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <map>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctime>


bool Procesrequest(Client * client, int epoll_fd, std::map<int, CgiProcess> &cgiByReadFd, std::map<int, int> &writeFdToReadFd);
int calculate_index(int current_fd, const std::map<int, const ServerConfig*> &listenFds, epoll_event ep);
void finishResponse(std::map<int, Client> &clients, int current_fd, int epoll_fd);
void close_conection(std::map<int, Client> &clients, int current_fd, int epoll_fd);
void checkClientTimeut(std::map<int, Client> &clients, int epoll_fd);
int	check_extension(const std::string &configPath);
std::string statusMessage(const std::string &code);

#endif
