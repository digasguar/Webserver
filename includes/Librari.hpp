#ifndef LIBRARI_H
# define LIBRARI_H
#pragma once

class Client;
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void Procesrequest(Client * client);
int calculate_index(int current_fd, int fd, epoll_event ep);

#endif