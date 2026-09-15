
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
    std::map<int, Client>::iterator it = clients.find(current_fd);
    if (it == clients.end())
        return ;

    char buffer[4094];

    Client& client = it->second;

    int bytes = recv(current_fd, buffer, sizeof(buffer), 0);
    if (bytes <= 0)
    {
        close_conection(clients, current_fd, epoll_fd);
        return ;
    }

    client.updateActivity();

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
            {
                chunk = "0\r\n\r\n";
                close(client.getFileFd());
                client.setFileFd(-1);
            }
            client.setBuffer(chunk.c_str(), chunk.size());
            client.setFileSize(chunk.size());
            client.setFileOffset(0);
            return (1);
        }
        ssize_t bytes = read(client.getFileFd(), client.getBuffer(), 4096);
        if (bytes <=0)
        {
            finishResponse(clients, current_fd, epoll_fd); //aqui habia un closed connection, pero finishResponse hace que el keep alive fncione en este caso
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

	////////////////////////
	//para qye no haga FAILURE BIND
	int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	////////////////////////
	
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
    std::map<int, Client>::iterator it = clients.find(current_fd);
    if (it == clients.end())
        return ;
    //std::cout << "ENTRO EN EPOLLOUT FD: " << current_fd << std::endl;
    Client& client = it->second;
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
    signal(SIGPIPE, SIG_IGN);
    //(ipv4, TCP, protocolo con 0 el sistema lo eligue por ti)
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        std::cout << "FAILURE CREATE SOCKET" << std::endl;
        exit(EXIT_FAILURE); 
    }
    prepare_socket(fd);
    int epoll_fd = epoll_create(42);
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
        int n = epoll_wait(epoll_fd, events, 42, 1000);
        if (n == -1)
        {
            perror("epoll wait");
            exit(EXIT_FAILURE);
        }
        checkClientTimeut(clients, epoll_fd);
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
