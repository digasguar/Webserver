#include "../includes/Librari.hpp"

std::string createResponse(const std::string body, const std::string type, const std::string status)
{

    std::stringstream ss;
    ss << body.size();
    std::string contentLength = ss.str();
    return ("HHTTP/1.1 " + status + "\r\n"
        "Content-Type: " + type + "\r\n"
        "Content-Length: " + contentLength + "\r\n"
        "Conection: close\r\n"
        "\r\n" + 
        body); 
}

std::string requestGet(Client * client)
{
    std::string path = client->getRequest().path;

    if (path == "/")
        path = "/index.html";
    std::string filePath = "/sgoinfre/students/dgasco-g/webserv/html/" + path;

    std::ifstream file(filePath.c_str(), std::ios::binary);

    if (!file.is_open())
    {
        std::string errorPath = "/sgoinfre/students/dgasco-g/webserv/html/404.jpg";
        std::ifstream errorFile(errorPath.c_str(), std::ios::binary);
        std::stringstream buffer;
        buffer << errorFile.rdbuf();

        std::string body = buffer.str();
        return (createResponse(body, "image/jpeg", "404 Not Found"));
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string body = buffer.str();
    return (createResponse(body, "text/html", "200 OK"));
}


std::string requestPost(Client * client)
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
    return createResponse("Archivo creado correctamente", "text/plain", "201 Created");
}

std::string requestDelete(Client *client)
{
    std::string path = client->getRequest().path;
    
    std::string filePath = "/sgoinfre/students/dgasco-g/webserv/html/" + path;
     
}

std::string Procesrequest(Client * client)
{

    if (client->getRequest().type == "GET")
    {
        return (requestGet(client));
    }
    else if (client->getRequest().type == "POST")
    {
        return(requestPost(client));
    }
    return (createResponse("418 Soy una tetera","text/plain", "418 Soy una tetera"));
}

