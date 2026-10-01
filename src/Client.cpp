#include "../includes/Client.hpp"
#include "../includes/ConfigTypes.hpp"
#include <cctype>
#include <cstring>
#include <cctype>
#include <cstdlib>


std::string toLower(const std::string &s)
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

static bool isTokenChar(unsigned char c)
{
    if (std::isalnum(c))
        return (true);
    return (c != 0 && std::strchr("!#$%&'*+-.^_`|~", c) != NULL);
}

// true si hay caracteres de control. En el valor de una cabecera el tabulador si esta permitido
static bool hasControlChars(const std::string &s, bool allowTab)
{
    for (size_t i = 0; i < s.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if ((c < 0x20 && !(allowTab && c == '\t')) || c == 0x7f)
            return (true);
    }
    return (false);
}

// 0 = valido (out relleno), 400 = no es un numero, 413 = demasiado grande para representarse.
// Antes se usaba atol(): "abc" daba 0 y "-5" daba un size_t enorme.
static int parseContentLength(const std::string &v, size_t &out)
{
    if (v.empty())
        return (400);
    for (size_t i = 0; i < v.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(v[i])))
            return (400);
    if (v.size() > 18)
        return (413);
    out = std::strtoul(v.c_str(), NULL, 10);
    return (0);
}

// una peticion mal formada: se guarda el codigo y se da por "terminada" para que Procesrequest conteste el error
void Client::failParse(int code)
{
    this->_parseError = code;
    this->_parseState = DONE;
}


Client::Client(int socket, const ServerConfig *serverConfig): _socket(socket), _serverConfig(serverConfig)
{
    static unsigned long nextSerial = 0;
    this->_serial = ++nextSerial;
    this->_file_fd = -1;
    this->_headerOffset = 0;
    this->_fileOffset = 0;
    this->_fileSize = 0;
    this->_isRegularFile = true;
    this->_parseState = LINE;
    this->_keep_alive = true;
    this->_parseError = 0;
    this->_last_activity = std::time(NULL);
    this->_buffer.resize(4096);
    this->_pipelined = false;
    this->_headerBytes = 0;
    
    if (serverConfig != NULL)
    {
        std::stringstream port;
        port << serverConfig->port;
        this->_request.serverPort = port.str();
    }
};

bool Client::takePipelined()
{
    bool was = this->_pipelined;
    this->_pipelined = false;
    return (was);
}

int Client::getSocket(){ return (this->_socket); };

HttpRequesr Client::getRequest(){ return (this->_request); };

Client::~Client(){};

void Client::setRequestType(const std::string& type) {this->_request.type = type;}

void Client::setRequestPath(const std::string& path){this->_request.path = path;}

void Client::setRequestQuery(const std::string& query){this->_request.query = query;}

void Client::setRequestVersion(const std::string& version){this->_request.version = version;}

void Client::setRecuestBody(const std::string& body){this->_request.body = body;}

void Client::setResponseHeaders(const std::string& headers){this->_responseHeaders = headers;}

void Client::setFileFd(const int fd){this->_file_fd = fd;}

void Client::setHeaderOffset(const size_t offset){this->_headerOffset = offset;}

void Client::setFileOffset(const off_t offset){this->_fileOffset = offset;}

void Client::setIsRegularFile(const bool regular){this->_isRegularFile = regular;}

void Client::setFileSize(const size_t size){this->_fileSize = size;}

 // ahora evita overflow, si algun dia hace falta mas de 4096 (tamaño de buffer)
 // entonces se cambia sin problemas
void Client::setBuffer(const char *buffer, size_t size)
{
    if (size > this->_buffer.size())
		this->_buffer.resize(size);   // antes se truncaba a 4096 y send() leia fuera del buffer
	std::copy(buffer, buffer + size, this->_buffer.begin());
}

void Client::setKeepAlive(const bool k){this->_keep_alive = k;}

void Client::setEpollEvent(const struct epoll_event &ep){this->_ep = ep;}

size_t Client::getHeaderOffset(){return this->_headerOffset;}

char * Client::getBuffer()
{
	if (this->_buffer.size() < 4096)   //Failsafe: nunca devolver un puntero a un vector vacio
		this->_buffer.resize(4096);
	return(&this->_buffer[0]);
}

off_t Client::getFileOffset(){return this->_fileOffset;}

bool Client::getIsRegularFile(){return this->_isRegularFile;}

off_t Client::getFileSize(){return this->_fileSize;}

int Client::getFileFd(){return this->_file_fd;}

std::string Client::getResponseHeaders(){return this->_responseHeaders;}

bool Client::getKeepAlive(){return (this->_keep_alive);}

const ServerConfig *Client::getServerConfig() const {return (this->_serverConfig);}

bool Client::isRequestComplete()
{
    return (_parseState == DONE);
}

void Client::setParseError(int code) { this->_parseError = code; }

int  Client::getParseError() { return this->_parseError; }

void Client::setRequestPathInfo(const std::string &pathInfo){this->_request.pathInfo = pathInfo;}

void Client::setRemoteAddr(const std::string &addr){this->_request.remoteAddr = addr;}

void Client::parseRequest()
{
    if (_parseState == LINE)
    {
        // RFC 7230 3.5: se ignoran las lineas vacias que haya antes de la request-line
        while (recv_buffer.compare(0, 2, "\r\n") == 0)
            recv_buffer.erase(0, 2);

        size_t pos = recv_buffer.find("\r\n");
        if (pos == std::string::npos)
        {
            // sin salto de linea y ya demasiado largo: si no, un cliente podria llenar la memoria sin limite
            if (recv_buffer.size() > MAX_REQUEST_LINE)
                failParse(414);
            return;
        }
        if (pos > MAX_REQUEST_LINE)
        {
            failParse(414);
            return;
        }

        std::string line = recv_buffer.substr(0, pos);
        recv_buffer.erase(0, pos + 2);
        _headerBytes = pos + 2;

        if (hasControlChars(line, false))
        {
            failParse(400);
            return;
        }

        std::istringstream iss(line);
        std::string type, path, version, extra;
        iss >> type >> path >> version;
        if (type.empty() || path.empty() || version.empty() || (iss >> extra))
        {
            failParse(400); // tienen que ser exactamente 3 partes: METODO RUTA VERSION
            return;
        }
        for (size_t i = 0; i < type.size(); ++i)
        {
            if (!isTokenChar(static_cast<unsigned char>(type[i])))
            {
                failParse(400);
                return;
            }
        }
        if (path[0] != '/')
        {
            failParse(400);
            return;
        }
        // "HTTP/" digito "." digito. Mal formada -> 400; bien formada pero que no soportamos -> 505
        if (version.size() != 8 || version.compare(0, 5, "HTTP/") != 0
            || !std::isdigit(static_cast<unsigned char>(version[5])) || version[6] != '.'
            || !std::isdigit(static_cast<unsigned char>(version[7])))
        {
            failParse(400);
            return;
        }
        if (version != "HTTP/1.1" && version != "HTTP/1.0")
        {
            failParse(505);
            return;
        }

		//////////////////////
		//el split para el query
		std::string query;
		size_t qpos = path.find('?');
		if (qpos != std::string::npos)
		{
			query = path.substr(qpos + 1);
			path = path.substr(0, qpos);
		}
		//////////////////////

		
		///////////
		//true decoded path
		path = percentDecode(path);
		///////////

		if (path.find('\0') != std::string::npos) // %00 en la ruta
        {
            failParse(400);
            return;
        }

        setRequestType(type);
        setRequestPath(path);
        setRequestQuery(query);
        setRequestVersion(version);

        _parseState = HEADERS;
    }

    if (_parseState == HEADERS)
    {
        while (true)
        {
            size_t pos = recv_buffer.find("\r\n");
            if (pos == std::string::npos)
            {
                // cabecera sin terminar y ya pasado el limite total
                if (_headerBytes + recv_buffer.size() > MAX_HEADER_BYTES)
                    failParse(431);
                return;
            }
            _headerBytes += pos + 2;
            if (_headerBytes > MAX_HEADER_BYTES)
            {
                failParse(431);
                return;
            }

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

				std::map<std::string, std::string>::iterator clIt = this->_request.headers.find("content-length");
                std::map<std::string, std::string>::iterator teIt = this->_request.headers.find("transfer-encoding");

                // RFC 7230 3.3.3: las dos a la vez es la base del "request smuggling" -> se rechaza
                if (clIt != this->_request.headers.end() && teIt != this->_request.headers.end())
                {
                    failParse(400);
                    return;
                }
                if (teIt != this->_request.headers.end())
                {
                    if (toLower(teIt->second) != "chunked")
                    {
                        failParse(501); // otros transfer-coding (gzip...) no los soportamos
                        return;
                    }
                    _parseState = BODY_CHUNKED;
                }
                else if (clIt != this->_request.headers.end())
                {
                    size_t len = 0;
                    int r = parseContentLength(clIt->second, len);
                    if (r != 0)
                    {
                        failParse(r);
                        return;
                    }
                    _parseState = BODY;
                }
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
            if (colon == std::string::npos || colon == 0)
            {
                failParse(400); // antes una cabecera sin ':' se ignoraba en silencio
                return;
            }

            std::string key = line.substr(0, colon);
            for (size_t i = 0; i < key.size(); ++i)
            {
                // un espacio antes de ':' o una linea que empieza por espacio (obs-fold) -> 400 (RFC 7230 3.2.4)
                if (!isTokenChar(static_cast<unsigned char>(key[i])))
                {
                    failParse(400);
                    return;
                }
            }
            std::string value = line.substr(colon + 1);
            size_t first = value.find_first_not_of(" \t");
            size_t last = value.find_last_not_of(" \t");
            value = (first == std::string::npos) ? std::string() : value.substr(first, last - first + 1);
            if (hasControlChars(value, true))
            {
                failParse(400);
                return;
            }

            key = toLower(key);
            std::map<std::string, std::string>::iterator ex = this->_request.headers.find(key);
            if (ex == this->_request.headers.end())
            {
                if (this->_request.headers.size() >= MAX_HEADER_COUNT)
                {
                    failParse(431);
                    return;
                }
                this->_request.headers[key] = value;
            }
            else if (key == "content-length" || key == "host" || key == "transfer-encoding")
            {
                if (ex->second != value) // duplicada y con valores distintos: ambigua -> 400
                {
                    failParse(400);
                    return;
                }
            }
            else
                ex->second += (key == "cookie" ? "; " : ", ") + value; // RFC 7230 3.2.2: se unen
        }
        if (_parseState == HEADERS)
            return;
    }

    if (_parseState == BODY)
    {
		size_t expectedLen = std::strtoul(this->_request.headers["content-length"].c_str(), NULL, 10); // ya validado como solo digitos
        size_t maxBodySize = (this->_serverConfig != NULL) ? this->_serverConfig->clientMaxBodySize : MAX_BODY_SIZE;

		if (expectedLen > maxBodySize)
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
		        if (recv_buffer.size() < pos + 4) // FIX: esperar a tener "0\r\n\r\n" completo ANTES de borrar nada
		            return;
		        recv_buffer.erase(0, pos + 4);

		        setRecuestBody(_chunkedBody);
		        _parseState = DONE;
		        break;
		    }
	//para evitar riesgo de overflow al ser sin signo si el cliente manda un chunksize muy grande
	//mas _chunkedbody podria dar la vuelta y pasar < maxBodySize. 
		    {
		        size_t maxBodySize = (this->_serverConfig != NULL) ? this->_serverConfig->clientMaxBodySize : MAX_BODY_SIZE;
		        if (chunkSize > maxBodySize || _chunkedBody.size() > maxBodySize - chunkSize)
		        {
		            setParseError(413);
		            _parseState = DONE;
		            return;
		        }
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
    //this->recv_buffer.clear(); NO SE VACIA, porque puede contener ya la siguiente peticion
    this->_request.body.clear();
    this->_request.path.clear();
    this->_request.query.clear();
    this->_request.type.clear();
    this->_request.version.clear();
    this->_request.headers.clear();
    this->_responseHeaders.clear();
    this->_keep_alive = true;
    this->_parseState = LINE;
    this->_ep.events = EPOLLIN;
    this->_chunkedBody.clear();
    this->_parseError = 0;
    this->_pipelined = false;
    this->_headerBytes = 0;
    this->_request.pathInfo.clear();
    if (this->_buffer.size() > 4096)
		std::vector<char>(4096).swap(this->_buffer);
	if (!this->recv_buffer.empty())
    {
        this->parseRequest(); // si ya hay una peticion entera esperando, queda lista para procesarse
        if (this->_parseState == DONE)
            this->_pipelined = true;
    }
}

void Client::updateActivity(){this->_last_activity = time(NULL);}

time_t Client::getLastActivity() const {return (this->_last_activity);}

unsigned long Client::getSerial() const { return this->_serial; }

