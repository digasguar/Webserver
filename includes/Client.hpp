#include <string>
#include "HttpRequest.hpp"

class Client
{
private:

    int _socket;

    HttpRequesr _request;

public:

    std::string recv_buffer;

    int getSocket();
    HttpRequesr getRequest();

    Client(int socket);
    ~Client();
};
