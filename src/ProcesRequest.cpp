#include "../includes/Librari.hpp"

std::string createResponse(const std::string body, const std::string type, const std::string status);

std::string Procesrequest(Client * client)
{
    if (client->getRequest().type == "GET")
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
    return (createResponse("418 Soy una tetera","text/plain", "418 Soy una tetera"));
}

std::string createResponse(const std::string body, const std::string type, const std::string status)
{
    return ("HHTTP/1.1 " + status + "\r\n"
        "Content-Type: " + type + "\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Conection: close\r\n"
        "\r\n" + 
        body); 
}