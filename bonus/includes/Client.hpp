#ifndef CLIENT_HPP
# define CLIENT_HPP
#include <string>
#include <vector>
#include "Librari.hpp"
#include "HttpRequest.hpp"
#define MAX_BODY_SIZE 1024 * 1024 * 10  // 10MB, fallback defensivo si _serverConfig fuera NULL (no deberia pasar nunca)
#define CLIENT_TIMEOUT 120 //tiempo para el timeut por inactividad (igual que el keepalive_timeout por defecto de nginx)
#define MAX_PIPELINE_BUFFER 65536 // lo maximo que se guarda de peticiones que llegan mientras se atiende otra

#define MAX_REQUEST_LINE 8192   // request-line mas larga aceptada -> 414 (nginx: 8k)
#define MAX_HEADER_BYTES 16384  // request-line + todas las cabeceras -> 431
#define MAX_HEADER_COUNT 100    // numero maximo de cabeceras -> 431

struct ServerConfig;


enum ClientState
{
    READING_REQUEST,
    WRITING_RESPONSE,
    FINISHED
};

enum ParseState
{
    LINE,
    HEADERS,
    BODY,
    BODY_CHUNKED,
    DONE
};

class Client
{
private:
    int _socket;

    HttpRequesr _request;//la peticion parseada de cliente

    ClientState _state; //estado del cliente

    ParseState _parseState;

    int _parseError;
    
    size_t _headerBytes; // bytes que llevan gastados la request-line + cabeceras de la peticion en curso

    std::string _responseHeaders; //los headers de la respuesta

    size_t _headerOffset; // cuanto hemos enviado de los headers

    bool _isRegularFile; //para el urandom y pipes

    off_t _fileSize; // cuanto pesa el archivo

    std::vector<char> _buffer; // el contenido del archivo (crece si el body de un CGI pasa de 4096)

    off_t _fileOffset;// por donde nos hemos quedado del archivo

    int _file_fd; //que archivo es

    bool _keep_alive; // mantener conexiones aviertas

	std::string _chunkedBody;

    struct epoll_event _ep;

    time_t _last_activity;
    
    unsigned long _serial;

    const ServerConfig *_serverConfig; // a que ServerConfig pertenece esta conexion, para el lookup de location

public:
    std::string recv_buffer;

    int getSocket();
    HttpRequesr getRequest();

    void setResponseHeaders(const std::string &headers);
    void setRequestType(const std::string &type);
    void setRequestPath(const std::string &path);
    void setRequestVersion(const std::string &version);
    void setRecuestBody(const std::string &body);
    void setFileFd(const int fd);
    void setHeaderOffset(const size_t offset);
    void setFileOffset(const off_t offset);
    void setIsRegularFile(const bool regular);
    void setFileSize(const size_t size);
    void setBuffer(const char *buffer, size_t size);
    void setKeepAlive(const bool k);
    void setEpollEvent(const struct epoll_event &ep);

    size_t getHeaderOffset();
    char *getBuffer();
    off_t getFileOffset();
    bool getIsRegularFile();
    off_t getFileSize();
    int getFileFd();
    std::string getResponseHeaders();
    bool getKeepAlive();
    struct epoll_event getEpollEvent();
    const ServerConfig *getServerConfig() const;

    void resetRequest();
    void parseRequest();
    bool isRequestComplete();
    void setParseError(int code);
    int  getParseError();
    void failParse(int code); // marca error de parseo y da la peticion por terminada
    
    void setRequestQuery(const std::string &query);
    void setRequestPathInfo(const std::string &pathInfo);
    void setRemoteAddr(const std::string &addr);

    void updateActivity();
    time_t getLastActivity() const;
    
    unsigned long getSerial() const;

    bool hasValidSesion(CookiesManager &cookieManager);

	bool _pipelined; // al acabar la respuesta anterior ya habia otra peticion COMPLETA en recv_buffer
	bool takePipelined(); // devuelve _pipelined y lo pone a false

    Client(int socket, const ServerConfig *serverConfig);
    ~Client();
};

std::string toLower(const std::string &s);

#endif
