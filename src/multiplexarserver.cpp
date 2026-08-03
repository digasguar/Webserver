
#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"


int main()
{
    //(ipv4, TCP, protocolo con 0 el sistema lo eligue por ti)
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        std::cout << "FAILURE CREATE SOCKET" << std::endl;
        exit(EXIT_FAILURE); 
    }

    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(8080);
    sockaddr.sin_addr.s_addr = INADDR_ANY; // acepta peticiones de cualquier interfaz de red

    if (bind(fd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) // asociar el puerto al soker
    {
        std::cout << "FAILURE BIND" << std::endl;
        exit(EXIT_FAILURE);
    }

    // (que fd escucha, numero de peticiones antes que se bloquee)
    if (listen(fd, 42) < 0) // escucha xd
    {
        std::cout << "FAILURE LISTEN" << std::endl;
        exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        std::cout << "FAILURE EPOLL\n";
        exit(EXIT_FAILURE);
    }
    epoll_event event ;
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
                sockaddr_in client;
                socklen_t len = sizeof(client);
                int fd_client = accept(fd,(struct sockaddr*)&client, &len);
                if (fd_client < 0)
                    continue;
                fcntl(fd_client, F_SETFL, O_NONBLOCK);
                clients.insert(std::make_pair(fd_client, Client(fd_client)));
                epoll_event client_event;
                client_event.data.fd = fd_client;
                client_event.events = EPOLLIN;

                epoll_ctl(epoll_fd,EPOLL_CTL_ADD, fd_client, &client_event);
                std::cout << "nuevo cliente creado" << std::endl;
            }
            else
            {
                std::cout << "asjha\n";
                if (events[i].events & EPOLLIN)
                {
                    char buffer[4094];

                    int bytes = recv(current_fd, buffer, sizeof(buffer), 0);
                    if (bytes <= 0)
                    {
                        clients.erase(current_fd);
                        close(current_fd);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                        continue;
                    }
                    Client& client = clients.at(current_fd);

                    client.recv_buffer.append(buffer, bytes);
                    //hardcodeado
                    client.setRequestType("GET");
                    client.setRequestPath("/index.html");
                    client.setRequestVersion("http1.1");
                    client.setRecuestBody("hola que tal?");

                    Procesrequest(&client);
                    epoll_event response_event;
                    response_event.data.fd = current_fd;
                    response_event.events = EPOLLOUT;
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &response_event);
                }
                else if(events[i].events & EPOLLOUT)
                {
                    std::cout << "ENTRO EN EPOLLOUT FD: " << current_fd << std::endl;
                    Client& client = clients.at(current_fd);
                    std::string headers = client.getResponseHeaders();
                    if (client.getHeaderOffset() < headers.size())
                    {
                        std::cout << "HEADERS A ENVIAR:\n";
                        std::cout << headers << "\n";
                        ssize_t sent = send(current_fd, headers.c_str() + client.getHeaderOffset(), headers.size() - client.getHeaderOffset(),0);
                        if (sent <= 0)
                        {
                            clients.erase(current_fd);
                            close(current_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                            continue;
                        }
                        client.setHeaderOffset(client.getHeaderOffset() + sent);
                        if (client.getHeaderOffset() < headers.size())
                            continue;
                    }
                    if (client.getFileOffset() == client.getFileSize())
                    {
                        ssize_t bytes = read(client.getFileFd(), client.getBuffer(), 4096);
                        if (bytes <=0)
                        {
                            clients.erase(current_fd);
                           // close(client.getFileFd());
                            close(current_fd);
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                            continue;
                        }
                        client.setFileSize(bytes);
                        client.setFileOffset(0);
                    }
                    std::cout << "envio :" << client.getFileSize() - client.getFileOffset();
                    std::cout << "--------------------------------------------------------------------------------------";
                    ssize_t sent = send(current_fd, client.getBuffer() + client.getFileOffset() ,client.getFileSize() - client.getFileOffset(),0 );
                    if (sent <= 0)
                    {
                        close(current_fd);
                        close(client.getFileFd());
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
                        continue;
                    }
                    client.setFileOffset(client.getFileOffset() + sent);
                    if (client.getFileOffset() < client.getFileSize())
                        continue;
                    
                }
            }
       }
    }
    close(fd);
}