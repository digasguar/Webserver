
#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"

void createClient(std::map<int, Client> &clients, int fd, int epoll_fd)
{
    sockaddr_in client;
    socklen_t len = sizeof(client);
    int fd_client = accept(fd,(struct sockaddr*)&client, &len);
    if (fd_client < 0)
        return ;
    fcntl(fd_client, F_SETFL, O_NONBLOCK);
    clients.insert(std::make_pair(fd_client, Client(fd_client)));
    epoll_event client_event;
    client_event.data.fd = fd_client;
    client_event.events = EPOLLIN;

    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd_client, &client_event);
    return ;
}

void reciveRequest(std::map<int, Client> &clients, int current_fd, int epoll_fd)
{
    char buffer[4094];

    int bytes = recv(current_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0)
    {
        clients.erase(current_fd);
        close(current_fd);
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, current_fd, NULL);
        return ;
    }
    Client& client = clients.at(current_fd);

    client.recv_buffer.append(buffer, bytes);

    client.parseRequest();
    if (!client.isRequestComplete())
    	return ;
    Procesrequest(&client);
    epoll_event response_event;
    response_event.data.fd = current_fd;
    response_event.events = EPOLLOUT;
    client.setEpollEvent(response_event);
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, current_fd, &response_event);
    return ;
}

int prepare_response(std::map<int, Client> &clients, Client &client, int current_fd, int epoll_fd)
{
    if (client.getFileFd() != -1)
    {
        if (!client.getIsRegularFile())
        {
            std::string buffer;
            buffer.resize(4080);
            ssize_t bytes = read(client.getFileFd(), &buffer[0], buffer.size());
            std::string chunk;
            if (bytes > 0)
            {
                std::stringstream ss;
                ss << std::hex << bytes;
                chunk = ss.str() + "\r\n";
                chunk.append(buffer.data(), bytes);
                chunk.append("\r\n");
            }
            else
                chunk = "0\r\n\r\n";
            client.setBuffer(chunk.c_str(), chunk.size());
            client.setFileSize(chunk.size());
            client.setFileOffset(0);
            return (1);
        }
        ssize_t bytes = read(client.getFileFd(), client.getBuffer(), 4096);
        if (bytes <=0)
        {
            close_conection(clients, current_fd, epoll_fd);
            return (0);
        }
        client.setFileSize(bytes);
        client.setFileOffset(0);
    }
    return (1);
}

void sendHeaders(int current_fd, std::string headers, Client &client, std::map<int , Client> &clients, int epoll_fd)
{
    ssize_t sent = send(current_fd, headers.c_str() + client.getHeaderOffset(), headers.size() - client.getHeaderOffset(),0);
    if (sent <= 0)
    {
        close_conection(clients, current_fd, epoll_fd);
        return ;
    }
    client.setHeaderOffset(client.getHeaderOffset() + sent);
}

void prepare_socket(int fd)
{
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
    if (listen(fd, 42) < 0)
    {
        std::cout << "FAILURE LISTEN" << std::endl;
        exit(EXIT_FAILURE);
    }
}

void sendResponse(std::map<int, Client> &clients, int current_fd, int epoll_fd)
{
    //std::cout << "ENTRO EN EPOLLOUT FD: " << current_fd << std::endl;
    Client& client = clients.at(current_fd);
    std::string headers = client.getResponseHeaders();
    if (client.getHeaderOffset() < headers.size())
    {
        sendHeaders(current_fd,headers,client,clients, epoll_fd);
        return ;
    }
    if (client.getFileOffset() == client.getFileSize())
    {
        if (client.getFileFd() == -1)
        {
            finishResponse(clients, current_fd, epoll_fd);
            return ;
        }
        if (!prepare_response(clients, client, current_fd, epoll_fd))
            return ;                       
    }
    ssize_t sent = send(current_fd, client.getBuffer() + client.getFileOffset() ,client.getFileSize() - client.getFileOffset(), 0);
    if (sent <= 0)
    {
        finishResponse(clients, current_fd, epoll_fd);
        close(client.getFileFd());
        return ;
    }
    client.setFileOffset(client.getFileOffset() + sent);
    return ;
}

void dummy(std::map<int, Client> &clients, int fd, int epoll_fd)
{
    (void)clients;
    (void)fd;
    (void)epoll_fd;
}

int main()
{
    //(ipv4, TCP, protocolo con 0 el sistema lo eligue por ti)
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        std::cout << "FAILURE CREATE SOCKET" << std::endl;
        exit(EXIT_FAILURE); 
    }
    prepare_socket(fd);
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
        void (*functions[])(std::map<int, Client> &, int, int) =
        {
            dummy,
            createClient,
            reciveRequest,
            sendResponse
        };
       for (int i = 0; i < n; i++)
        {
            int current_fd = events[i].data.fd;
            size_t index = calculate_index(current_fd, fd, events[i]);
            functions[index](clients, current_fd, epoll_fd);
        }
    }
    close(fd);
}
