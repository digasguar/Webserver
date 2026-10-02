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

#include <set>

// fds cerrados o creados durante la vuelta actual del bucle. epoll_wait ya devolvio sus eventos, asi que cualquier
// evento pendiente para uno de estos numeros es de un fd ANTIGUO (el kernel reutiliza los numeros) y hay que ignorarlo.
extern std::set<int> g_touchedFds;
void closeTracked(int fd); // close() que ignora eventos viejos de epoll sobre ese numero en esta vuelta

bool Procesrequest(Client * client, int epoll_fd, std::map<int, CgiProcess> &cgiByReadFd, std::map<int, int> &writeFdToReadFd);
int calculate_index(int current_fd, const std::map<int, const ServerConfig*> &listenFds, epoll_event ep);
void finishResponse(std::map<int, Client> &clients, int current_fd, int epoll_fd);
void close_conection(std::map<int, Client> &clients, int current_fd, int epoll_fd);
void checkClientTimeut(std::map<int, Client> &clients, int epoll_fd);
int	check_extension(const std::string &configPath);
std::string statusMessage(const std::string &code);
// respuesta de error unica (honra "error_page <code>" para cualquier codigo; ver ProcesRequest.cpp)
void setErrorResponse(Client *client, int code, const std::string &detail = "",
                      const std::string &extraHeaders = "", const std::string &fallbackPage = "");

#endif
