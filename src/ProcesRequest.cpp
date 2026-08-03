#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"

std::string createHeadersLength(const std::string type, const std::string status, size_t length)
{
    std::stringstream ss;
    ss << length;
    return ("HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + type + "\r\n"
        "Content-Length: " + ss.str() + "\r\n"
        "Conection: close\r\n"
        "\r\n");
}

std::string createChungedHeader(const std::string type, const std::string status)
{
    return ("HTTP/1.1 " + status + "\r\n"
        "Content-Type: " + type + "\r\n"
        "Transfer-Encoding: chunked" + "\r\n"
        "Conection: close\r\n"
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
        path = "/dev/urandom"; //pepe el urandom no porfavor, cualquier cosa menos eso 
        int errorfd = open(path.c_str(), O_RDONLY); // no se si con el archivo de configuracion podriamos hacer que la paguina de conf siempre se pueda abrir
        stat(path.c_str(), &st);// esto tambien puede fallar, deveria prevenirlo pero me da pereza
        if (S_ISREG(st.st_mode) != 0)
            client->setResponseHeaders(createHeadersLength(typeFile, "404", st.st_size));
        else
            client->setResponseHeaders(createChungedHeader(typeFile, "404"));
        client->setFileFd(errorfd);
        return ;
    }
    if (stat(filePath.c_str(), &st) < 0)
    {
        ; 
    }
    if (S_ISREG(st.st_mode) != 0)
        client->setResponseHeaders(createHeadersLength(typeFile, "200", st.st_size));
    else
        client->setResponseHeaders(createChungedHeader(typeFile, "200"));
    client->setFileFd(file);
}


























/*
std::string createHeader(const std::string type, const std::string status, bool special_size)
{
    std::stringstream size;
    size << contentLength;
    if (special_size)
    {

    }
    return ("HHTTP/1.1 " + status + "\r\n"
        "Content-Type: " + type + "\r\n"
        "Content-Length: " + size.str() + "\r\n"
        "Conection: close\r\n"
        "\r\n");
}

void requestGet(Client * client)
{
    std::string path = client->getRequest().path;

    if (path == "/")
        path = "/index.html";
    std::string filePath = "/sgoinfre/students/dgasco-g/webserv/html/" + path;

    std::ifstream file(filePath.c_str(), std::ios::binary);

    if (!file.is_open())
    {
        std::string path = "/dev/urandom"; 
        int errorPath = open(path.c_str(), O_RDONLY);
        struct stat st;
        stat(path.c_str(), &st);
        if (S_ISREG(st.st_mode))
        {
            clie
        }

        std::ifstream errorFile(errorPath.c_str(), std::ios::binary);
        std::stringstream buffer;
        buffer << errorFile.rdbuf();

        std::string body = buffer.str();
        createResponse(body, "image/jpeg", "404 Not Found");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string body = buffer.str();
    return (createResponse(body, "text/html", "200 OK"));
}


void requestPost(Client * client)
{
    std::string path = client->getRequest().path;

    std::string filePath = "/sgoinfre/students/dgasco-g/webserv/html/" + path;

    std::ofstream file(filePath.c_str(), std::ios::binary);
    if (!file.is_open())
    {
        return (createResponse("500 INternal server error","text/plain", "500 internal server error"));
    }
    file << client->getRequest().body;
    file.close();
    //return createResponse("Archivo creado correctamente", "text/plain", "201 Created");
}

void requestDelete(Client *client)
{
    std::string path = client->getRequest().path;
    
    std::string filePath = "/sgoinfre/students/dgasco-g/webserv/html/" + path;
    //return("hola");
}
 */

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

