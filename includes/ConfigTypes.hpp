#ifndef CONFIGTYPES_HPP
# define CONFIGTYPES_HPP
#include "Client.hpp"
#include <string>
#include <vector>
#include <map>

struct LocationConfig
{
    std::string path;
    std::string root;
    std::vector<std::string> methods;
    bool autoindex;
    std::string index;
    std::string uploadStore;
    std::map<std::string, std::string> cgiHandlers; // extension -> interprete

    LocationConfig()
    {
        this->root = "./html";
        this->index = "index.html";
        this->methods.push_back("GET");
        this->autoindex = false;
    }
};

struct ServerConfig
{
    std::string host;
    int port;
    size_t clientMaxBodySize;
    std::map<int, std::string> errorPages;
    std::vector<LocationConfig> locations;

    ServerConfig()
    {
        this->host = "0.0.0.0";
        this->port = -1;
        this->clientMaxBodySize = MAX_BODY_SIZE;
    }
};

typedef std::vector<ServerConfig> Config; //para soportar multiples servers.

#endif
