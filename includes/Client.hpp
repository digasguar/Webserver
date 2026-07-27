#include <string>

class Client
{
private:

    int _socket;

public:

    std::string recv_buffer;

    int getSocket();
    Client(int socket);
    ~Client();
};
