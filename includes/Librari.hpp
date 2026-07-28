#ifndef LIBRARI_H
# define LIBRARI_H
#include "Client.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <map>
#include <fstream>
#include <sstream>


std::string Procesrequest(Client * client);

#endif