#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>

int main()
{
    //(ipv4, TCP, protocolo con 0 el sistema lo eligue por ti)
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        std::cout << "FAILURE CREATE SOCKET" << errno <<std::endl;
        exit(EXIT_FAILURE); 
    }

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(8080);
    sockaddr.sin_addr.s_addr = INADDR_ANY; // acepta peticiones de cualquier interfaz de red

    if (bind(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) // asociar el puerto al soker
    {
        std::cout << "FAILURE BIND" << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    // (que fd escucha, numero de peticiones antes que se bloquee)
    if (listen(fd, 42) < 0) // escucha xd
    {
        std::cout << "FAILURE LISTEN" << errno << std::endl;
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        sockaddr_in client {};
        socklen_t len = sizeof(client);

        //(sodkket del server, cliente que se ha conectado , len del la estructura del cliente)
        int client_fd = accept(fd, (struct sockaddr*)&client, &len);//procesar peticion
        if (client_fd < 0)
            continue;
        char buffer[4096];

        //(socket del cliente, donde se guardan los datos, maximo de datos que recibe, ni puta idea de que es esto xd)
        recv(client_fd, buffer,size_t(buffer),0);//recibe la info del socket del cliente 
        std::string body = "Hola desde el server";
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;
        
        // (fd del cliente al que contesta, respuesta, cuanto ocupa la respuesta , ni puta idea)
        send(client_fd,response.c_str(),response.size(),0);//que respuesta da el srvr
        close(client_fd);
    }
    close(fd);
}