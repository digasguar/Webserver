#include "../includes/Client.hpp"

Client::Client(int socket): _socket(socket){};

int Client::getSocket(){ return (this->_socket); };

HttpRequesr Client::getRequest(){ return (this->_request); };

Client::~Client(){};

void Client::setRequestType(const std::string& type) {this->_request.type = type;}

void Client::setRequestPath(const std::string& path){this->_request.path = path;}

void Client::setRequestVersion(const std::string& version){this->_request.version = version;}




