#include "../includes/Client.hpp"

Client::Client(int socket): _socket(socket)
{
    this->_file_fd = -1;

    this->_bytesSent = 0;
    this->_headerSend = false;
    this->_bytes_read = 0;
};

int Client::getSocket(){ return (this->_socket); };

HttpRequesr Client::getRequest(){ return (this->_request); };

Client::~Client(){};

void Client::setRequestType(const std::string& type) {this->_request.type = type;}

void Client::setRequestPath(const std::string& path){this->_request.path = path;}

void Client::setRequestVersion(const std::string& version){this->_request.version = version;}

void Client::setRecuestBody(const std::string& body){this->_request.body = body;}

void Client::setResponseHeaders(const std::string& headers){this->_responseHeaders = headers;}

void Client::setFileFd(const int fd){this->_file_fd = fd;} 




