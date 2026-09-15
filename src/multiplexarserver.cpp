
#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"
#include "../includes/ConfigTypes.hpp"
#include "../includes/ConfigTokenizer.hpp"
#include "../includes/ConfigParser.hpp"
#include <cstring>

#define DEFAULT_CONFIG_PATH "./config/default.conf"

void createClient(std::map<int, Client> &clients, int fd, int epoll_fd, const std::map<int, const ServerConfig*> &listenFds)
{
    sockaddr_in client;
    socklen_t len = sizeof(client);
    int fd_client = accept(fd,(struct sockaddr*)&client, &len);
    if (fd_client < 0)
        return ;
    fcntl(fd_client, F_SETFL, O_NONBLOCK);

    std::map<int, const ServerConfig*>::const_iterator srvIt = listenFds.find(fd);
    const ServerConfig *serverConfig = NULL;
    if (srvIt != listenFds.end())
        serverConfig = srvIt->second;

    clients.insert(std::make_pair(fd_client, Client(fd_client, serverConfig)));
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

    char buffer[4094]; //porque esto y no 4096?

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
                chunk = "0\r\n\r\n";
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
    client.updateActivity();
}

void prepare_socket(int fd, const std::string &host, int port)
{
    std::stringstream portStream;
    portStream << port;
    std::string portStr = portStream.str();

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // por si algun dia host llega vacio, resuelve a "todas las interfaces"
    struct addrinfo *res = NULL;
    int status = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res);
    if (status != 0)
    {
        std::cout << "FAILURE GETADDRINFO: " << gai_strerror(status) << std::endl;
        exit(EXIT_FAILURE);
    }

	////////////////////////
	//para qye no haga FAILURE BIND
	int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	////////////////////////

    if (bind(fd, res->ai_addr, res->ai_addrlen) < 0)
    {
        std::cout << "FAILURE BIND" << std::endl;
        freeaddrinfo(res);
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(res);

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
        close(client.getFileFd());
        return ;
    }
    client.setFileOffset(client.getFileOffset() + sent);
    client.updateActivity();
    return ;
}

void dummy(std::map<int, Client> &clients, int fd, int epoll_fd)
{
    (void)clients;
    (void)fd;
    (void)epoll_fd;
}

static std::string resolveConfigPath(int argc, char **argv)
{
    if (argc > 2)
    {
        std::cerr << "Usage: ./webserv [config_file]" << std::endl;
        exit(EXIT_FAILURE);
    }
    if (argc == 2)
        return (std::string(argv[1]));
    return (std::string(DEFAULT_CONFIG_PATH));
}

static Config loadConfig(const std::string &configPath)
{
    std::vector<std::string> tokens;
    if (!tokenizeConfigFile(configPath, tokens))
    {
        std::cerr << "Error: could not open config file '" << configPath << "'" << std::endl;
        exit(EXIT_FAILURE);
    }

    Config config;
    std::string parseError;
    if (!parseConfig(tokens, config, parseError))
    {
        std::cerr << "Error: " << parseError << std::endl;
        exit(EXIT_FAILURE);
    }
    return (config);
}

// crea y bindea un socket de escucha por cada ServerConfig, todos bajo el mismo epoll_fd
static std::map<int, const ServerConfig*> setupListenSockets(const Config &config, int epoll_fd)
{
    std::map<int, const ServerConfig*> listenFds;

    for (size_t i = 0; i < config.size(); ++i)
    {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1)
        {
            std::cout << "FAILURE CREATE SOCKET" << std::endl;
            exit(EXIT_FAILURE);
        }
        prepare_socket(fd, config[i].host, config[i].port);

        epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event) == -1)
        {
            perror("ERROR epoll_ctl");
            exit(EXIT_FAILURE);
        }
        listenFds[fd] = &config[i];
    }
    return (listenFds);
}

int main(int argc, char **argv)
{
    signal(SIGPIPE, SIG_IGN);
    // si se hace send() a un socket que ya cerró la conexion el kernel devuelve SIGPIPE que mata todo el proceso.

    std::string configPath = resolveConfigPath(argc, argv);
    Config config = loadConfig(configPath);

    int epoll_fd = epoll_create(42);
    if (epoll_fd == -1)
    {
        std::cout << "FAILURE EPOLL\n";
        exit(EXIT_FAILURE);
    }

    std::map<int, const ServerConfig*> listenFds = setupListenSockets(config, epoll_fd);

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
            dummy, // el indice 1 (nueva conexion) se maneja aparte con createClient, porque necesita listenFds
            reciveRequest,
            sendResponse
        };
        for (int i = 0; i < n; i++)
        {
            int current_fd = events[i].data.fd;
            size_t index = calculate_index(current_fd, listenFds, events[i]);
            if (index == 1)
                createClient(clients, current_fd, epoll_fd, listenFds);
            else
                functions[index](clients, current_fd, epoll_fd);
        }
    }
    for (std::map<int, const ServerConfig*>::iterator it = listenFds.begin(); it != listenFds.end(); ++it)
        close(it->first);
}
