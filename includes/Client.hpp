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
    void setRequestType(const std::string& type);
    void setRequestPath(const std::string& path);
    void setRequestVersion(const std::string& version);
    
    Client(int socket);
    ~Client();
    
    ////////////////////
    void parseRequest();
    ////////////////////
};
