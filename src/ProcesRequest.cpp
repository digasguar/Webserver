#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"

std::string createHeadersLength(const std::string type, const std::string status, size_t length)
{
    std::stringstream ss;
    ss << length;
    return ("HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + type + "\r\n"
        "Content-Length: " + ss.str() + "\r\n"
        "Connection: close\r\n"
        "\r\n");
}

std::string createChunkedHeader(const std::string type, const std::string status)
{
    return ("HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + type + "\r\n"
        "Transfer-Encoding: chunked" + "\r\n"
        "Connection: close\r\n"
        "\r\n");
}

void requestGet(Client *client)
{
    std::string path = client->getRequest().path;

    if (path == "/")
        path = "/index.html";
    std::string filePath = "/sgoinfre/students/dgasco-g/webserv/html/" + path;
    std::string typeFile;
    if (path.find(".html") != std::string::npos)
        typeFile = "text/html";
    else if (path.find(".jpg") != std::string::npos)
        typeFile = "image/jpeg";
    else 
        typeFile = "text/plain";
    int file = open(filePath.c_str(), O_RDONLY);
    struct stat st;
    if (file < 0)
    { 
        int errorfd = open(path.c_str(), O_RDONLY); // no se si con el archivo de configuracion podriamos hacer que la paguina de conf siempre se pueda abrir
        stat(path.c_str(), &st);// esto tambien puede fallar, deveria prevenirlo pero me da pereza
        if (S_ISREG(st.st_mode) != 0)
            client->setResponseHeaders(createHeadersLength(typeFile, "404", st.st_size));
        else
        {
            client->setIsRegularFile(false);
            client->setResponseHeaders(createChunkedHeader(typeFile, "404"));
        }
        client->setFileFd(errorfd);
        close(file);
        return ;
    }
    stat(filePath.c_str(), &st);
    if (S_ISREG(st.st_mode) != 0)
        client->setResponseHeaders(createHeadersLength(typeFile, "200", st.st_size));
    else
        client->setResponseHeaders(createChunkedHeader(typeFile, "200"));
    client->setFileFd(file);
}

void requestPost(Client *client)
{
    std::string path = client->getRequest().path;

    std::string filePath = "/sgoinfre/students/dgasco-g/webserv/html/" + path;

    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        client->setResponseHeaders(createHeadersLength("text/plain", "200", 0));
    }
    file << client->getRequest().body;
    file.close();
}


void Procesrequest(Client * client)
{

    if (client->getRequest().type == "GET")
    {
        return (requestGet(client));
    }
/*     else if (client->getRequest().type == "POST")
    {
        return(requestPost(client));
    }
    return (createHeader("418 Soy una tetera","text/plain")); */
}

