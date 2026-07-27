#include "../includes/Client.hpp"

Client::Client(int socket): _socket(socket){};

int Client::getSocket(){ return (this->_socket); };

Client::~Client(){};

