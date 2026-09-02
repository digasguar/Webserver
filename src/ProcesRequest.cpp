#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"

#include <dirent.h>

#define AUTOINDEX_ENABLED true  // pendiente: vendra del archivo de configuracion

std::string createHeadersLength(const std::string type, const std::string status, size_t length, bool keep_alive)
{
    std::stringstream ss;
    ss << length;

    if (!keep_alive)
        return ("HTTP/1.1 " + status + "\r\n"
            "Content-Type: " + type + "\r\n"
            "Content-Length: " + ss.str() + "\r\n"
            "Connection: close\r\n"
            "\r\n");
    return ("HTTP/1.1 " + status + "\r\n"
            "Content-Type: " + type + "\r\n"
            "Content-Length: " + ss.str() + "\r\n"
            "Connection: keep-alive\r\n"
            "\r\n");
}

std::string createChunkedHeader(const std::string type, const std::string status, bool keep_alive)
{
    if (!keep_alive)
        return ("HTTP/1.1 " + status + "\r\n"
            "Content-Type: " + type + "\r\n"
            "Transfer-Encoding: chunked" + "\r\n"
            "Connection: close\r\n"
            "\r\n");
    return ("HTTP/1.1 " + status + "\r\n"
            "Content-Type: " + type + "\r\n"
            "Transfer-Encoding: chunked" + "\r\n"
            "Connection: keep-alive\r\n"
            "\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////
// creamos un index.html que iría en el directorio objetivo, pero las tripas las hacemos un string a secas,
// y en lugar de cagarlo en el directorio, lo guardamos en un tempfile, que luego se borrará, ara que no quede rastro

std::string generateAutoindexHTML(const std::string &dirFsPath, const std::string &urlPath)
{
    std::stringstream html;
    html << "<html><head><title>Index of " << urlPath << "</title></head><body>";
    html << "<h1>Index of " << urlPath << "</h1><ul>";

    DIR *dir = opendir(dirFsPath.c_str());
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;
            if (name == ".")
                continue;
            html << "<li><a href=\"" << name << "\">" << name << "</a></li>";
        }
        closedir(dir);
    }
    html << "</ul></body></html>";
    return (html.str());
}

int writeAutoindexToTempFile(const std::string &html)
{
    char tmpPath[] = "/tmp/webserv_autoindex_XXXXXX";
    int fd = mkstemp(tmpPath);
    if (fd < 0)
        return (-1);

    ssize_t written = write(fd, html.c_str(), html.size());
	if (written < 0 || static_cast<size_t>(written) != html.size())
	{
		unlink(tmpPath);
		close(fd);
		return (-1);
	}
    unlink(tmpPath);
    lseek(fd, 0, SEEK_SET);
    return (fd);
}

/////////////////////////////////////////////////////////////////////////////////////////////////

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
            client->setResponseHeaders(createHeadersLength(typeFile, "404", st.st_size, client->getKeepAlive()));
        else
        {
            client->setIsRegularFile(false);
            client->setResponseHeaders(createChunkedHeader(typeFile, "404", client->getKeepAlive()));
        }
        client->setFileFd(errorfd);
        close(file);
        return ;
    }
    stat(filePath.c_str(), &st);
    ////////////////////////////////////////////////////////////////////////
    if (S_ISDIR(st.st_mode))
	{
		close(file);

		std::string indexPath = filePath;
		if (indexPath[indexPath.size() - 1] != '/')
		    indexPath += "/";
		indexPath += "index.html";

		int indexFd = open(indexPath.c_str(), O_RDONLY);
		if (indexFd >= 0)
		{
		    struct stat indexSt;
		    stat(indexPath.c_str(), &indexSt);
		    client->setResponseHeaders(createHeadersLength("text/html", "200", indexSt.st_size, client->getKeepAlive()));
		    client->setFileFd(indexFd);
		    return;
		}

		if (AUTOINDEX_ENABLED)
		{
		    std::string listing = generateAutoindexHTML(filePath, path);
		    int listingFd = writeAutoindexToTempFile(listing);
		    client->setResponseHeaders(createHeadersLength("text/html", "200", listing.size(), client->getKeepAlive()));
		    client->setFileFd(listingFd);
		    return;
		}

		std::string body = "Forbidden";
		client->setResponseHeaders(createHeadersLength("text/plain", "403 Forbidden", body.size(), client->getKeepAlive()));
		client->setBuffer(body.c_str(), body.size());
		client->setFileOffset(0);
		client->setIsRegularFile(true);
		client->setFileSize(body.size());
		client->setFileFd(-1);
		return;
	}
    ///////////////////////////////////////////////////////////////////////////////
    if (S_ISREG(st.st_mode) != 0)
        client->setResponseHeaders(createHeadersLength(typeFile, "200", st.st_size, client->getKeepAlive()));
    else
        client->setResponseHeaders(createChunkedHeader(typeFile, "200", client->getKeepAlive()));
    client->setFileFd(file);
}

void requestPost(Client *client)
{
    std::string path = client->getRequest().path;

    std::string filePath = "./html" + path;
    client->setFileFd(-1);
    if (path.find("../") != std::string::npos)
    {
        std::string body = "Forbidden";
        client->setResponseHeaders(createHeadersLength("text/plain", "403 Forbidden", body.size(), client->getKeepAlive()));
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
    client->setResponseHeaders(createHeadersLength("text/plain",status, body.size(), client->getKeepAlive()));
    client->setBuffer(body.c_str(),body.size());
    client->setFileOffset(0);
    client->setIsRegularFile(true);
    client->setFileSize(body.size());
}

void requestDelete(Client *client)
{
    std::string path = client->getRequest().path;
    std::string filePath = "./html" + path;
    std::string body;
    std::string status;

    client->setFileFd(-1);
    if (filePath.find("../") != std::string::npos)
    {
        body = "Forbidden";
        status = "403 Forbidden";
    }
    else
    {
        struct stat st;
        if (stat(filePath.c_str(), &st) != 0)
        {
            body = "Not Found";
            status = "404 Not Found";
        }
        else if (!S_ISREG(st.st_mode))
        {
            body = "Forbidden";
            status = "403 Forbidden";
        }
        else if (std::remove(filePath.c_str()) != 0)
        {
            body = "Could not delete file";
            status = "500 Internal Server Error";
        }
        else
        {
            body = "Deleted";
            status = "200 OK";
        }
    }
    client->setResponseHeaders(createHeadersLength("text/plain",status, body.size(), client->getKeepAlive()));
    client->setBuffer(body.c_str(),body.size());
    client->setFileOffset(0);
    client->setIsRegularFile(true);
    client->setFileSize(body.size());
}

void Procesrequest(Client * client)
{
	if (client->getParseError() != 0)
    {
        std::string status, body;

        if (client->getParseError() == 411)
        {
            status = "411 Length Required";
            body = "Length Required";
        }
        else if (client->getParseError() == 413)
        {
            status = "413 Payload Too Large";
            body = "Payload Too Large";
        }

        client->setResponseHeaders(createHeadersLength("text/plain", status, body.size(), client->getKeepAlive()));
        client->setBuffer(body.c_str(), body.size());
        client->setFileOffset(0);
        client->setIsRegularFile(true);
        client->setFileSize(body.size());
        return;
    }
    
    /////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////
    //	SE PUEDE BORRAR E N EL FUTURO
    
    std::cout << "=== REQUEST COMPLETE ===\n"
          << "type: " << client->getRequest().type << "\n"
          << "path: " << client->getRequest().path << "\n"
          << "body: [" << client->getRequest().body << "]\n"
          << "=========================\n";

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////

    if (client->getRequest().type == "GET")
        return (requestGet(client));
    else if (client->getRequest().type == "POST")
        return (requestPost(client));
    else if (client->getRequest().type == "DELETE")
        return (requestDelete(client));
}
