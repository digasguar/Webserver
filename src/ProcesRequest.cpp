#include "../includes/Librari.hpp"

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
            std::string error = "404 Not Found";
            return (
                "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: " + std::to_string(error.size()) + "\r\n"
                "Connection: close\r\n"
                "\r\n" +
                error);
        }
        std::string body = "8================D";
        std::string response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            body;
        return (response);

    }
    std::string body = "418 Soy una tetera";
    std::string response =
        "HTTP/1.1 418 Soy una tetera\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" +
        body;
    return (response);
}