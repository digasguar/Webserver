#include "../includes/Client.hpp"
#include <cctype>

#define MAX_BODY_SIZE 10000000  // 10MB, como limite de body, esto iria en el archivo de configuracion


static std::string toLower(const std::string &s)
{
    std::string result = s;
    for (size_t i = 0; i < result.size(); ++i)
        result[i] = std::tolower(static_cast<unsigned char>(result[i]));
    return result;
}

//para decodear los %20 %3C %2f y asi
static std::string percentDecode(const std::string &s)
{
    std::string out;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '%' && i + 2 < s.size()
            && std::isxdigit(static_cast<unsigned char>(s[i + 1]))
            && std::isxdigit(static_cast<unsigned char>(s[i + 2])))
        {
            std::string hex = s.substr(i + 1, 2);
            int value = std::strtol(hex.c_str(), NULL, 16);
            out += static_cast<char>(value);
            i += 2;
        }
        else
            out += s[i];
    }
    return out;
}




Client::Client(int socket): _socket(socket)
{
    this->_file_fd = -1;
    this->_headerOffset = 0;
    this->_fileOffset = 0;
    this->_fileSize = 0;
    this->_isRegularFile = true;
    this->_parseState = LINE;
    this->_keep_alive = true; 
    this->_parseError = 0;
    this->_last_activity = std::time(NULL);
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

void Client::setEpollEvent(const struct epoll_event &ep){this->_ep = ep;}

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

void Client::setParseError(int code) { this->_parseError = code; }

int  Client::getParseError() { return this->_parseError; }

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

		///////////
		//true decoded path
		path = percentDecode(path);
		///////////

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
				bool isHttp11 = (this->_request.version == "HTTP/1.1");
				std::map<std::string, std::string>::iterator connIt =
					this->_request.headers.find("connection");

				if (connIt == this->_request.headers.end())
					setKeepAlive(isHttp11);
				else
					setKeepAlive(toLower(connIt->second) == "keep-alive");

				bool hasContentLength = this->_request.headers.count("content-length") > 0;
				bool hasChunked = this->_request.headers.count("transfer-encoding") > 0;

				if (hasContentLength)
					_parseState = BODY;
				else if (hasChunked)
					_parseState = BODY_CHUNKED;
				else if (this->_request.type == "POST")
				{
					setParseError(411);
					_parseState = DONE;
				}
				else
					_parseState = DONE;
				break;
			}

            size_t colon = line.find(':');
            if (colon == std::string::npos)
                continue;

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            if (!value.empty() && value[0] == ' ')
                value.erase(0, 1);

            this->_request.headers[toLower(key)] = value;
        }
        if (_parseState == HEADERS)
            return;
    }

    if (_parseState == BODY)
    {
        size_t expectedLen = std::atol(this->_request.headers["content-length"].c_str());

		if (expectedLen > MAX_BODY_SIZE)
		{
			setParseError(413);
			_parseState = DONE;
			return;
		}

        if (recv_buffer.size() < expectedLen)
            return;

        std::string body = recv_buffer.substr(0, expectedLen);
        recv_buffer.erase(0, expectedLen);
        setRecuestBody(body);

        _parseState = DONE;
    }
    
    if (_parseState == BODY_CHUNKED)
{
    while (true)
    {
        size_t pos = recv_buffer.find("\r\n");
        if (pos == std::string::npos)
            return;

        std::string sizeLine = recv_buffer.substr(0, pos);
        size_t chunkSize = std::strtoul(sizeLine.c_str(), NULL, 16);

        if (chunkSize == 0)
        {
            recv_buffer.erase(0, pos + 2);
            if (recv_buffer.size() < 2)
                return;
            recv_buffer.erase(0, 2);

            setRecuestBody(_chunkedBody);
            _parseState = DONE;
            break;
        }

        if (recv_buffer.size() < pos + 2 + chunkSize + 2)
            return;

        std::string chunkData = recv_buffer.substr(pos + 2, chunkSize);
        _chunkedBody += chunkData;
        recv_buffer.erase(0, pos + 2 + chunkSize + 2);
    }
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
    this->_request.headers.clear();
    this->_responseHeaders.clear();
    this->_keep_alive = true;
    this->_parseState = LINE;
    this->_ep.events = EPOLLIN;
    this->_chunkedBody.clear();
    this->_parseError = 0;
}

void Client::updateActivity(){this->_last_activity = time(NULL);}

time_t Client::getLastActivity() const {return (this->_last_activity);}
