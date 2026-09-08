#include "../includes/Librari.hpp"
#include "../includes/Client.hpp"

#include <cstring>
#include <dirent.h>
#include <vector>
#include <algorithm>

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

// en general para las redirecciones (ha surgido de un bug al hacer GET a un directorio)
std::string createRedirectHeader(const std::string &location, bool keep_alive) // para cuando la ruta del directorio no acaba en "/", le decimos al cliente que la buena es con "/" y le redirigimos
{
    if (!keep_alive)
        return ("HTTP/1.1 301 Moved Permanently\r\n"
            "Location: " + location + "\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n"
            "\r\n");
    return ("HTTP/1.1 301 Moved Permanently\r\n"
            "Location: " + location + "\r\n"
            "Content-Length: 0\r\n"
            "Connection: keep-alive\r\n"
            "\r\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////
// creamos un index.html que iría en el directorio objetivo, pero las tripas las hacemos un string a secas,
// y en lugar de cagarlo en el directorio, lo guardamos en un tempfile, que luego se borrará, ara que no quede rastro

struct DirEntry
{
    std::string name;
    bool isDir;
};

static bool compareDirEntries(const DirEntry &a, const DirEntry &b)
{
    if (a.isDir != b.isDir)
        return (a.isDir); // directories first
    return (a.name < b.name); // alphabetical within the same type
}


std::string generateAutoindexHTML(const std::string &dirFsPath, const std::string &urlPath)
{
    std::vector<DirEntry> entries;

    DIR *dir = opendir(dirFsPath.c_str());
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            std::string name = entry->d_name;
            if (name == ".")
                continue;

            std::string fullPath = dirFsPath;
            if (fullPath[fullPath.size() - 1] != '/')
                fullPath += "/";
            fullPath += name;

            struct stat entrySt;
            DirEntry de;
            de.name = name;
            de.isDir = (stat(fullPath.c_str(), &entrySt) == 0 && S_ISDIR(entrySt.st_mode));
            entries.push_back(de);
        }
        closedir(dir);
    }

    std::sort(entries.begin(), entries.end(), compareDirEntries);

    std::stringstream html;
    html << "<html><head><title>Index of " << urlPath << "</title></head><body>";
    html << "<h1>Index of " << urlPath << "</h1><ul>";
    for (size_t i = 0; i < entries.size(); ++i)
    {
        std::string suffix = entries[i].isDir ? "/" : "";
        html << "<li><a href=\"" << entries[i].name << suffix << "\">"
             << entries[i].name << suffix << "</a></li>";
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


// para manejar el .. y que no se salga dela carpeta ./html, o la que sea ene l config
// GET y DELETE
static bool isPathWithinRoot(const std::string &fsPath, const std::string &root)
{
    char realRoot[PATH_MAX];
    char realTarget[PATH_MAX];

    if (realpath(root.c_str(), realRoot) == NULL)
        return false;
    if (realpath(fsPath.c_str(), realTarget) == NULL)
        return true;

    std::string realRootStr(realRoot);
    std::string realTargetStr(realTarget);

    if (realTargetStr == realRootStr)
        return true;
    if (realTargetStr.size() > realRootStr.size() &&
        realTargetStr.compare(0, realRootStr.size(), realRootStr) == 0 &&
        realTargetStr[realRootStr.size()] == '/')
        return true;
    return false;
}

// para POST, checkeamos que la capeta exista, y asi poder crear el archivo de ser necesario
static bool isCreateTargetWithinRoot(const std::string &fsPath, const std::string &root)
{
    size_t slash = fsPath.find_last_of('/');
    std::string parentDir = (slash == std::string::npos) ? "." : fsPath.substr(0, slash);
    std::string filename = (slash == std::string::npos) ? fsPath : fsPath.substr(slash + 1);

    if (filename == ".." || filename == "." || filename.empty())
        return false;

    char realRoot[PATH_MAX];
    char realParent[PATH_MAX];

    if (realpath(root.c_str(), realRoot) == NULL)
        return false;
    if (realpath(parentDir.c_str(), realParent) == NULL)
        return false;

    std::string realRootStr(realRoot);
    std::string realParentStr(realParent);

    if (realParentStr == realRootStr)
        return true;
    if (realParentStr.size() > realRootStr.size() &&
        realParentStr.compare(0, realRootStr.size(), realRootStr) == 0 &&
        realParentStr[realRootStr.size()] == '/')
        return true;
    return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////

void requestGet(Client *client)
{
    std::string path = client->getRequest().path;
    
    std::string filePath = "./html" + path;
    
    ///////////////////////////////////
    if (!isPathWithinRoot(filePath, "./html"))
	{
		std::string body = "Forbidden";
		client->setResponseHeaders(createHeadersLength("text/plain", "403 Forbidden", body.size(), client->getKeepAlive()));
		client->setBuffer(body.c_str(), body.size());
		client->setFileOffset(0);
		client->setIsRegularFile(true);
		client->setFileSize(body.size());
		client->setFileFd(-1);
		return ;
	}
    //////////////////////////////////
    
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
        close(file); // medidas extra de precaucion, por si acaso

		std::string errorPagePath = "./html/404.html"; // el archivo de 404
		struct stat errSt;
		memset(&errSt, 0, sizeof(errSt)); // inicializarlo en cero para que luego no pille valore basura

		int errorfd = open(errorPagePath.c_str(), O_RDONLY);
		if (errorfd >= 0 && stat(errorPagePath.c_str(), &errSt) == 0 && S_ISREG(errSt.st_mode))
		{
			client->setResponseHeaders(createHeadersLength("text/html", "404", errSt.st_size, client->getKeepAlive()));
			client->setFileFd(errorfd);
		}
		else
		{
			// por si acaso la pagina de 404 no se puede cargar o desaparece, ara que no pete
			if (errorfd >= 0)
				close(errorfd);
			std::string body = "Not Found";
			client->setResponseHeaders(createHeadersLength("text/plain", "404", body.size(), client->getKeepAlive()));
			client->setBuffer(body.c_str(), body.size());
			client->setFileOffset(0);
			client->setIsRegularFile(true);
			client->setFileSize(body.size());
			client->setFileFd(-1);
		}
		return;
    }
    stat(filePath.c_str(), &st);
    ////////////////////////////////////////////////////////////////////////
    if (S_ISDIR(st.st_mode))
	{
		close(file);

		if (path[path.size() - 1] != '/')
		{
		    client->setResponseHeaders(createRedirectHeader(path + "/", client->getKeepAlive()));
		    client->setBuffer("", 0);
		    client->setFileOffset(0);
		    client->setIsRegularFile(true);
		    client->setFileSize(0);
		    client->setFileFd(-1);
		    return;
		}

		std::string indexPath = filePath;
		if (indexPath[indexPath.size() - 1] != '/') // estas dos lineas creoq ue se pueden queitar porque el if anterior "fixea" que el cliente nos meta un directorio sin / al final, no lo hago porque tengo miedo, y sigue funcionando bien aun con el dead code
		    indexPath += "/";						// estas dos lineas creoq ue se pueden queitar porque el if anterior "fixea" que el cliente nos meta un directorio sin / al final, no lo hago porque tengo miedo, y sigue funcionando bien aun con el dead code
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
    if (!isCreateTargetWithinRoot(filePath, "./html")) //cambiado de path.find("../")
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
    if (!isPathWithinRoot(filePath, "./html")) //cambiado de path.find("../")
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

void requestNotAllowed(Client *client)
{
    std::string body = "405 Method Not Allowed"; 
    std::string status = "405";

    client->setResponseHeaders(createHeadersLength("text/plain", status, body.size(), client->getKeepAlive()));
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
    requestNotAllowed(client);
}
