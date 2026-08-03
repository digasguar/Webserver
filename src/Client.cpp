#include "../includes/Client.hpp"

Client::Client(int socket): _socket(socket)
{
    this->_file_fd = -1;
    this->_headerOffset = 0;
    this->_fileOffset = 0;
    this->_fileSize = 0;
    this->_isRegularFile = false;
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

void Client::setHeaderOffset(const size_t offset){this->_headerOffset = offset;}

void Client::setFileOffset(const off_t offset){this->_fileOffset = offset;}

void Client::setIsRegularFile(const bool regular){this->_isRegularFile = regular;}

void Client::setFileSize(const size_t size){this->_fileSize = size;}

void Client::setBuffer(const char *buffer, size_t size){std::copy(buffer, buffer + size, this->_buffer);}

size_t Client::getHeaderOffset(){return this->_headerOffset;}

char * Client::getBuffer(){return(this->_buffer);}

off_t Client::getFileOffset(){return this->_fileOffset;}

bool Client::getIsRegularFile(){return this->_isRegularFile;}

off_t Client::getFileSize(){return this->_fileSize;}

int Client::getFileFd(){return this->_file_fd;}

std::string Client::getResponseHeaders(){return this->_responseHeaders;}



