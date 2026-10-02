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
#include <exception>
#include "../includes/CookiesManager.hpp"

#include <set>

// fds cerrados o creados durante la vuelta actual del bucle. epoll_wait ya devolvio sus eventos, asi que cualquier
// evento pendiente para uno de estos numeros es de un fd ANTIGUO (el kernel reutiliza los numeros) y hay que ignorarlo.
extern std::set<int> g_touchedFds;
void closeTracked(int fd); // close() que ignora eventos viejos de epoll sobre ese numero en esta vuelta

static const std::string LOGIN_PAGE =
    "<!DOCTYPE html>\n"
    "<html lang=\"es\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "    <title>Login - Mi Webserv</title>\n"
    "    <style>\n"
    "        body {\n"
    "            background-color: #202020;\n"
    "            color: white;\n"
    "            font-family: Arial, sans-serif;\n"
    "            text-align: center;\n"
    "            margin-top: 100px;\n"
    "        }\n"
    "\n"
    "        h1 {\n"
    "            color: #00ff99;\n"
    "        }\n"
    "\n"
    "        .box {\n"
    "            border: 2px solid #00ff99;\n"
    "            padding: 20px 30px;\n"
    "            display: inline-block;\n"
    "            border-radius: 10px;\n"
    "        }\n"
    "\n"
    "        input[type=\"text\"] {\n"
    "            margin-top: 10px;\n"
    "            padding: 12px 16px;\n"
    "            background-color: #303030;\n"
    "            color: white;\n"
    "            font-size: 16px;\n"
    "            border: 2px solid #00ff99;\n"
    "            border-radius: 10px;\n"
    "            outline: none;\n"
    "            text-align: center;\n"
    "        }\n"
    "\n"
    "        input[type=\"text\"]:focus {\n"
    "            background-color: #3a3a3a;\n"
    "        }\n"
    "\n"
    "        .btn {\n"
    "            display: inline-block;\n"
    "            margin-top: 20px;\n"
    "            padding: 12px 28px;\n"
    "            background-color: #00ff99;\n"
    "            color: #202020;\n"
    "            font-size: 16px;\n"
    "            font-weight: bold;\n"
    "            cursor: pointer;\n"
    "            border: 2px solid #00ff99;\n"
    "            border-radius: 10px;\n"
    "            transition: background-color 0.2s, color 0.2s;\n"
    "        }\n"
    "\n"
    "        .btn:hover {\n"
    "            background-color: transparent;\n"
    "            color: #00ff99;\n"
    "        }\n"
    "    </style>\n"
    "</head>\n"
    "\n"
    "<body>\n"
    "    <div class=\"box\">\n"
    "        <h1>Inicia sesión</h1>\n"
    "        <p>Introduce tu nombre de usuario para entrar al servidor 🔐</p>\n"
    "        <form method=\"POST\" action=\"/login/submit\">\n"
    "            <input type=\"text\" name=\"username\" placeholder=\"nombre de usuario\" required autofocus>\n"
    "            <br>\n"
    "            <button class=\"btn\" type=\"submit\">Entrar</button>\n"
    "        </form>\n"
    "    </div>\n"
    "</body>\n"
    "</html>\n";

bool Procesrequest(Client * client, int epoll_fd, std::map<int, CgiProcess> &cgiByReadFd, std::map<int, int> &writeFdToReadFd, CookiesManager & cookiesManager);
int calculate_index(int current_fd, const std::map<int, const ServerConfig*> &listenFds, epoll_event ep);
void finishResponse(std::map<int, Client> &clients, int current_fd, int epoll_fd);
void close_conection(std::map<int, Client> &clients, int current_fd, int epoll_fd);
void checkClientTimeut(std::map<int, Client> &clients, int epoll_fd);
int	check_extension(const std::string &configPath);
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

// respuesta de error unica (honra "error_page <code>" para cualquier codigo; ver ProcesRequest.cpp)
void setErrorResponse(Client *client, int code, const std::string &detail = "",
                      const std::string &extraHeaders = "", const std::string &fallbackPage = "");

#endif
