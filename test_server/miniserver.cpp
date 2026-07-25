#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        std::cout << "FAILURE CREATE SOCKET" << errno <<std::endl;
        exit(EXIT_FAILURE); 
    }

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(8080);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0)
    {
        std::cout << "FAILURE BIND" << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 42) < 0)
    {
        std::cout << "FAILURE LISTEN" << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        sockaddr_in client {};
        socklen_t len = sizeof(client);

        int client_fd = accept(fd, (struct sockaddr*)&client, &len);
        if (client_fd < 0)
            continue;
        char buffer[4096];
        int n = recv(fd, buffer,size_t(buffer),0);
        std::string body = "Hola asier y manu que tal ?";
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;
        send(client_fd,response.c_str(),response.size(),0);
        if (n > 0)
        {
            buffer[n] = '\0';
            std::cout << buffer << std::endl;
        }
        close(client_fd);
    }
    close(fd);
}