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
#include "../includes/CookiesManager.hpp"


static const std::string LOGIN_PAGE =
    "<!DOCTYPE html><html><head><title>Login</title></head><body>"
    "<h1>Inicia sesion</h1>"
    "<form method=\"POST\" action=\"/login/submit\">"
    "<input type=\"text\" name=\"username\" placeholder=\"nombre de usuario\" required>"
    "<button type=\"submit\">Entrar</button>"
    "</form></body></html>"; // chatgepeteada historica XD

bool Procesrequest(Client * client, int epoll_fd, std::map<int, CgiProcess> &cgiByReadFd, std::map<int, int> &writeFdToReadFd, CookiesManager & cookiesManager);
int calculate_index(int current_fd, const std::map<int, const ServerConfig*> &listenFds, epoll_event ep);
void finishResponse(std::map<int, Client> &clients, int current_fd, int epoll_fd);
void close_conection(std::map<int, Client> &clients, int current_fd, int epoll_fd);
void checkClientTimeut(std::map<int, Client> &clients, int epoll_fd);
std::string statusMessage(const std::string &code);

std::string extractCookie(const std::string &cookieHeader);
bool isPublicRoute(const std::string &path);
void requestLoginSubmit(Client *client, CookiesManager &cookieManager);
void requestLoginPage(Client *client);
void requestRedirectToLogin(Client *client);

std::string extractFormField(const std::string &body, const std::string &field);
std::string createAuthRedirectWithCookie(const std::string &location, const std::string &cookieValue, bool keep_alive);
std::string createAuthRedirect(const std::string &location, bool keep_alive);
std::string createRedirectHeader(const std::string &location, bool keep_alive);
std::string createChunkedHeader(const std::string type, const std::string status, bool keep_alive);
std::string createHeadersLength(const std::string type, const std::string status, size_t length, bool keep_alive);

#endif
