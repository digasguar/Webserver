#include "../includes/Client.hpp"
#include <sstream>

Client::Client(int socket): _socket(socket){};

int Client::getSocket(){ return (this->_socket); };

HttpRequesr Client::getRequest(){ return (this->_request); };

Client::~Client(){};

void Client::setRequestType(const std::string& type) {this->_request.type = type;}

void Client::setRequestPath(const std::string& path){this->_request.path = path;}

void Client::setRequestVersion(const std::string& version){this->_request.version = version;}


////////////////////////////////////////////////////////////////////////////////////
void Client::parseRequest()
{
    size_t pos = recv_buffer.find("\r\n");
    if (pos == std::string::npos)
        return;

    std::string line = recv_buffer.substr(0, pos);
    recv_buffer.erase(0, pos + 2);

    std::istringstream iss(line);
    std::string type, path, version;
    iss >> type >> path >> version;

    setRequestType(type);
    setRequestPath(path);
    setRequestVersion(version);
}
