#include "../includes/Client.hpp"

#define MAX_BODY_SIZE 10000000  // 10MB, como limite de body, esto iria en el archivo de configuracion

Client::Client(int socket): _socket(socket)
{
    this->_file_fd = -1;
    this->_headerOffset = 0;
    this->_fileOffset = 0;
    this->_fileSize = 0;
    this->_isRegularFile = true;
    this->_parseState = LINE;
    this->_keep_alive = true; 
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

void Client::setKeepAlive(const bool k){this->_keep_alive = k;}

size_t Client::getHeaderOffset(){return this->_headerOffset;}

char * Client::getBuffer(){return(this->_buffer);}

off_t Client::getFileOffset(){return this->_fileOffset;}

bool Client::getIsRegularFile(){return this->_isRegularFile;}

off_t Client::getFileSize(){return this->_fileSize;}

int Client::getFileFd(){return this->_file_fd;}

std::string Client::getResponseHeaders(){return this->_responseHeaders;}

bool Client::getKeepAlive(){return (this->_keep_alive);}

bool Client::isRequestComplete()
{
    return (_parseState == DONE);
}

void Client::parseRequest()
{
    if (_parseState == LINE)
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

        _parseState = HEADERS;
    }

    if (_parseState == HEADERS)
    {
        size_t pos;
        while ((pos = recv_buffer.find("\r\n")) != std::string::npos)
        {
            std::string line = recv_buffer.substr(0, pos);
            recv_buffer.erase(0, pos + 2);

            if (line.empty())
            {
                std::map<std::string, std::string>::iterator it =
                    this->_request.headers.find("Content-Length");
                if (it == this->_request.headers.end())
                    _parseState = DONE;
                else
                    _parseState = BODY;
                break;
            }

            size_t colon = line.find(':');
            if (colon == std::string::npos)
                continue;

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            if (!value.empty() && value[0] == ' ')
                value.erase(0, 1);

            this->_request.headers[key] = value;
        }
        if (_parseState == HEADERS)
            return;
    }

    if (_parseState == BODY)
    {
        size_t expectedLen = std::atol(this->_request.headers["Content-Length"].c_str());

		if (expectedLen > MAX_BODY_SIZE)
		{
			_parseState = DONE;   // para para no explotar por un tamaño demasiado grande
			// pendiente: tiene que triggerear un 413
			return;
		}

        if (recv_buffer.size() < expectedLen)
            return;

        std::string body = recv_buffer.substr(0, expectedLen);
        recv_buffer.erase(0, expectedLen);
        setRecuestBody(body);

        _parseState = DONE;
    }
}

void Client::resetRequest()
{
    this->setFileFd(-1);
    this->setFileOffset(0);
    this->setFileSize(0);
    this->setIsRegularFile(true);
    this->setHeaderOffset(0);
    this->recv_buffer.clear();
    this->_request.body.clear();
    this->_request.path.clear();
    this->_request.type.clear();
    this->_request.version.clear();
    this->_responseHeaders.clear();
    this->_keep_alive = true;
}
