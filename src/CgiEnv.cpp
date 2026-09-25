#include "../includes/CgiEnv.hpp"
#include <sstream>
#include <cctype>

static std::string toUpperUnderscore(const std::string &headerName)
{
    std::string result;
    for (size_t i = 0; i < headerName.size(); ++i)
    {
        char c = headerName[i];
        if (c == '-')
            result += '_';
        else
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

std::vector<std::string> buildCgiEnv(const HttpRequesr &request, const std::string &scriptPath)
{
    std::vector<std::string> env;

    env.push_back("REQUEST_METHOD=" + request.type);
    env.push_back("SCRIPT_NAME=" + request.path);
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("QUERY_STRING=" + request.query);
    env.push_back("SERVER_PROTOCOL=" + request.version);
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("REDIRECT_STATUS=200"); // php-cgi lo exige (los otros CGI no), para que sea que puede acceder, porque las restricciones las tenemos en la request

    if (!request.body.empty())
    {
        std::stringstream len;
        len << request.body.size();
        env.push_back("CONTENT_LENGTH=" + len.str());
    }

    std::map<std::string, std::string>::const_iterator ctIt = request.headers.find("content-type");
    if (ctIt != request.headers.end())
        env.push_back("CONTENT_TYPE=" + ctIt->second);

    for (std::map<std::string, std::string>::const_iterator it = request.headers.begin();
         it != request.headers.end(); ++it)
    {
        if (it->first == "content-type" || it->first == "content-length")
            continue;
        env.push_back("HTTP_" + toUpperUnderscore(it->first) + "=" + it->second);
    }

    return env;
}
