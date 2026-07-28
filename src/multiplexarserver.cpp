
#include "../includes/Librari.hpp"


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

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        std::cout << "FAILURE EPOLL\n";
        exit(EXIT_FAILURE);
    }
    epoll_event event {};
    event.data.fd = fd;
    event.events = EPOLLIN;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
    {
        perror("ERROR epoll_ctl");
        exit(EXIT_FAILURE);
    }
    epoll_event events[42];
    std::map<int, Client> clients;
    while (1)
    {
       int n = epoll_wait(epoll_fd, events, 42, -1);
       if (n == -1)
       {
            perror("epoll wait");
            exit(EXIT_FAILURE);
       }
       for (int i = 0; i < n; i++)
       {
            int current_fd = events[i].data.fd;
            if (current_fd == fd)
            {
                sockaddr_in client {};
                socklen_t len = sizeof(client);
                int fd_client = accept(fd,(struct sockaddr*)&client, &len);
                if (fd_client < 0)
                    continue;
                clients.insert(std::make_pair(fd_client, Client(fd_client)));
                epoll_event client_event{};
                client_event.data.fd = fd_client;
                client_event.events = EPOLLIN;

                epoll_ctl(epoll_fd,EPOLL_CTL_ADD, fd_client, &client_event);
                std::cout << "nuevo cliente creado" << std::endl;
            }
            else
            {
                char buffer[4094];

                int bytes = recv(current_fd, buffer, sizeof(buffer), 0);
                if (bytes <= 0)
                {
                    close(current_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                    continue;
                }
                Client& client = clients.at(current_fd);

                client.recv_buffer.append(buffer, bytes);
                //hardcodeado
                client.setRequestType("GET");
                client.setRequestPath("/indexa.html");
                client.setRequestVersion("http1.1");

                Procesrequest(&client);

                std::string response = Procesrequest(&client);
                send(current_fd, response.data(), response.size(), 0);
                close(current_fd);
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
            }
       }
    }
    close(fd);
}