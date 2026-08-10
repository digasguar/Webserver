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
    std::string filePath = "./html" + path;
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

    std::string filePath = "./html" + path;

    if (path.find("..") != std::string::npos)
    {
        std::string body = "Forbidden";
        client->setResponseHeaders(createHeadersLength("text/plain", "403 Forbidden", body.size()));
        client->setBuffer(body.c_str(), body.size());
        client->setFileSize(body.size());
        client->setFileOffset(0);
        client->setIsRegularFile(true);
        return ;
    }
    struct stat st;
    bool exist = (stat(filePath.c_str(),&st) == 0);
    std::ofstream file(filePath.c_str(), std::ios::binary | std::ios::trunc);
    std::string body;
    std::string status;

    if (!file.is_open())
    {
        body = "Could not write file";
        status = "500 Internal Server Error";
    }
    else
    {
        file << client->getRequest().body;
        file.close();
        if (exist)
        {
            body = "Updated";
            status = "200 OK";
        }
        else
        {
            body = "Created";
            status = "201 Created";
        }
    }
    client->setResponseHeaders(createHeadersLength("text/plain",status, body.size()));
    client->setBuffer(body.c_str(),body.size());
    client->setFileOffset(0);
    client->setIsRegularFile(true);
    client->setFileSize(body.size());
}

void requestDelete(Client *client)
{
    std::string path = client->getRequest().path;

    std::string filePath = "./html" + path;

    if (std::remove(filePath.c_str()) != 0)
    {
       // client->setResponseHeaders(createHeadersLength())
    }
}

void Procesrequest(Client * client)
{

    std::cout << "=== REQUEST COMPLETE ===\n"
          << "type: " << client->getRequest().type << "\n"
          << "path: " << client->getRequest().path << "\n"
          << "body: [" << client->getRequest().body << "]\n"
          << "=========================\n";

    if (client->getRequest().type == "GET")
        return (requestGet(client));
    else if (client->getRequest().type == "POST")
        return (requestPost(client));
    else if (client->getRequest().type == "DELETE")
        return (requestDelete(client));
}
