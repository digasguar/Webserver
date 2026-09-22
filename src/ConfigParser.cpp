#include "../includes/ConfigParser.hpp"
#include <cstdlib>
#include <cctype>

static bool isValidNumber(const std::string &s)
{
    if (s.empty())
        return (0);
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            return (0);
    }
    return (1);
}

static bool readSingleArg(TokenCursor &cursor, std::string &value, std::string &err, const std::string &directiveName)
{
    if (!cursor.hasMore() || cursor.peek() == ";" || cursor.peek() == "{" || cursor.peek() == "}")
    {
        err = "directive '" + directiveName + "' expects a value";
        return (0);
    }
    value = cursor.advance();
    if (!cursor.hasMore() || cursor.peek() != ";")
    {
        err = "directive '" + directiveName + "' expects exactly one value, missing ';'";
        return (0);
    }
    cursor.advance();
    return (1);
}

static bool readTwoArgs(TokenCursor &cursor, std::string &a, std::string &b, std::string &err, const std::string &directiveName)
{
    if (!cursor.hasMore() || cursor.peek() == ";" || cursor.peek() == "{" || cursor.peek() == "}")
    {
        err = "directive '" + directiveName + "' expects two values";
        return (0);
    }
    a = cursor.advance();
    if (!cursor.hasMore() || cursor.peek() == ";" || cursor.peek() == "{" || cursor.peek() == "}")
    {
        err = "directive '" + directiveName + "' expects two values";
        return (0);
    }
    b = cursor.advance();
    if (!cursor.hasMore() || cursor.peek() != ";")
    {
        err = "directive '" + directiveName + "' expects exactly two values, missing ';'";
        return (0);
    }
    cursor.advance();
    return (1);
}

static bool readArgsUntilSemicolon(TokenCursor &cursor, std::vector<std::string> &args, std::string &err)
{
    while (true)
    {
        if (!cursor.hasMore())
        {
            err = "unexpected end of file, expected ';'";
            return (0);
        }
        const std::string &tok = cursor.peek();
        if (tok == ";")
        {
            cursor.advance();
            return (1);
        }
        if (tok == "{" || tok == "}")
        {
            err = "expected ';' but found '" + tok + "'";
            return (0);
        }
        args.push_back(tok);
        cursor.advance();
    }
}

static bool parseLocationBody(TokenCursor &cursor, LocationConfig &loc, std::string &err)
{
    while (true)
    {
        if (!cursor.hasMore())
        {
            err = "unexpected end of file, expected '}' to close location block";
            return (0);
        }
        if (cursor.peek() == "}")
        {
            cursor.advance();
            return (1);
        }

        std::string directive = cursor.advance();

        if (directive == "root")
        {
            if (!readSingleArg(cursor, loc.root, err, "root"))
                return (0);
        }
        else if (directive == "index")
        {
            if (!readSingleArg(cursor, loc.index, err, "index"))
                return (0);
        }
        else if (directive == "upload_store")
        {
            if (!readSingleArg(cursor, loc.uploadStore, err, "upload_store"))
                return (0);
        }
        else if (directive == "autoindex")
        {
            std::string value;
            if (!readSingleArg(cursor, value, err, "autoindex"))
                return (0);
            if (value == "on")
                loc.autoindex = true;
            else if (value == "off")
                loc.autoindex = false;
            else
            {
                err = "directive 'autoindex' expects 'on' or 'off', got '" + value + "'";
                return (0);
            }
        }
        else if (directive == "methods")
        {
            std::vector<std::string> args;
            if (!readArgsUntilSemicolon(cursor, args, err))
                return (0);
            if (args.empty())
            {
                err = "directive 'methods' expects at least one value";
                return (0);
            }
            loc.methods = args;
        }
        else if (directive == "cgi_extension")
        {
            std::string ext, interpreter;
            if (!readTwoArgs(cursor, ext, interpreter, err, "cgi_extension"))
                return (0);
            loc.cgiHandlers[ext] = interpreter;
        }
        else if (directive == "location")
        {
            err = "nested 'location' blocks are not supported";
            return (0); //se puede hacer aca volviendo a llamar a parseLocationBody() pero ensucia el config file.
        }
        else
        {
            err = "unknown directive '" + directive + "' inside location block";
            return (0);
        }
    }
}

static bool parseServerBody(TokenCursor &cursor, ServerConfig &srv, std::string &err)
{
    bool listenSet = false;

    while (1)
    {
        if (!cursor.hasMore())
        {
            err = "unexpected end of file, expected '}' to close server block";
            return (0);
        }
        if (cursor.peek() == "}")
        {
            cursor.advance();
            break;
        }

        std::string directive = cursor.advance();

        if (directive == "listen")
        {
            std::string value;
            if (!readSingleArg(cursor, value, err, "listen"))
                return (0);
            std::string host = srv.host;
            std::string portStr = value;
            size_t colon = value.find(':');
            if (colon != std::string::npos)
            {
                host = value.substr(0, colon);
                portStr = value.substr(colon + 1);
            }
            if (!isValidNumber(portStr))
            {
                err = "directive 'listen' expects a valid port number, got '" + value + "'";
                return (0);
            }
            int port = std::atoi(portStr.c_str());
            if (port <= 0 || port > 65535)
            {
                err = "directive 'listen' port out of range: " + portStr;
                return (0);
            }
            srv.host = host;
            srv.port = port;
            listenSet = true;
        }
        else if (directive == "client_max_body_size")
        {
            std::string value;
            if (!readSingleArg(cursor, value, err, "client_max_body_size"))
                return (0);
            if (!isValidNumber(value))
            {
                err = "directive 'client_max_body_size' expects a numeric value, got '" + value + "'";
                return (0);
            }
            srv.clientMaxBodySize = static_cast<size_t>(std::strtoul(value.c_str(), NULL, 10));
        }
        else if (directive == "error_page")
        {
            std::string codeStr, path;
            if (!readTwoArgs(cursor, codeStr, path, err, "error_page"))
                return (0);
            if (!isValidNumber(codeStr))
            {
                err = "directive 'error_page' expects a numeric status code, got '" + codeStr + "'";
                return (0);
            }
            int code = std::atoi(codeStr.c_str());
            srv.errorPages[code] = path;
        }
        else if (directive == "location")
        {
            if (!cursor.hasMore() || cursor.peek() == "{" || cursor.peek() == ";" || cursor.peek() == "}")
            {
                err = "directive 'location' expects a path";
                return (0);
            }
            std::string locPath = cursor.advance();
            if (!cursor.hasMore() || cursor.peek() != "{")
            {
                err = "expected '{' after location path '" + locPath + "'";
                return (0);
            }
            cursor.advance();

            LocationConfig loc;
            loc.path = locPath;
            if (!parseLocationBody(cursor, loc, err))
                return (0);
            srv.locations.push_back(loc);
        }
        else
        {
            err = "unknown directive '" + directive + "' inside server block";
            return (0);
        }
    }

    if (!listenSet)
    {
        err = "missing mandatory 'listen' directive in server block";
        return (0);
    }
    return (1);
}
//lee las directivas server a ver si hay mas de uno.
bool parseConfig(const std::vector<std::string> &tokens, Config &config, std::string &errorMessage)
{
    config.clear();
    TokenCursor cursor(tokens);

    if (!cursor.hasMore())
    {
        errorMessage = "empty configuration file";
        return (0);
    }
    while (cursor.hasMore())
    {
        std::string tok = cursor.advance();
        if (tok != "server")
        {
            errorMessage = "expected 'server' block, found '" + tok + "'";
            return (0);
        }
        if (!cursor.hasMore() || cursor.peek() != "{")
        {
            errorMessage = "expected '{' after 'server'";
            return (0);
        }
        cursor.advance();

        ServerConfig srv;
        if (!parseServerBody(cursor, srv, errorMessage))
            return (0);
        config.push_back(srv);
    }
    if (config.empty()) //creo que nunca llegaria aca pero por las dudas.
    {
        errorMessage = "no server blocks defined";
        return (0);
    }
    return (1);
}
