#ifndef CLIENT_HPP
# define CLIENT_HPP
#include <string>
#include "Librari.hpp"
#include "HttpRequest.hpp"
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
    DONE
};

class Client
{
private:
    int _socket;

    HttpRequesr _request;//la peticion parseada de cliente

    ClientState _state; //estado del cliente
    
    ParseState _parseState;

    std::string _responseHeaders; //los headers de la respuesta

    size_t _headerOffset; // cuanto hemos enviado de los headers

    bool _isRegularFile; //para el urandom y pipes

    off_t _fileSize; // cuanto pesa el archivo 

    char _buffer[4096]; // el contenido del archivo

    off_t _fileOffset;// por donde nos hemos quedado del archivo

    int _file_fd; //que archivo es

    bool _keep_alive; // mantener conexiones aviertas

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

    size_t getHeaderOffset();
    char *getBuffer();
    off_t getFileOffset();
    bool getIsRegularFile();
    off_t getFileSize();
    int getFileFd();
    std::string getResponseHeaders();
    bool getKeepAlive();

    void resetRequest();
    void parseRequest();
    bool isRequestComplete();
    
    Client(int socket);
    ~Client();
};
#endif
