#include <string>
#include "Librari.hpp"
#include "HttpRequest.hpp"

enum ClientState
{
    READING_REQUEST,
    WRITING_RESPONSE,
    FINISHED
};

class Client
{
private:
    int _socket;

    HttpRequesr _request;

    ClientState _state;

    std::string _responseHeaders;

    char _buffer[4096];

    int _file_fd;

    ssize_t _bytes_read;

    size_t _bytesSent;

    bool _headerSend;

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

    Client(int socket);
    ~Client();
};
