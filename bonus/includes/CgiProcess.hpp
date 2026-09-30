#ifndef CGIPROCESS_HPP
# define CGIPROCESS_HPP

#include <string>
#include <sys/types.h>
#include "HttpRequest.hpp"

struct CgiProcess
{
    pid_t pid;
    int   writeFd;
    int   readFd;
    int   clientFd;
    size_t bodyBytesSent;
    std::string outputSoFar;
    bool  finished;

    CgiProcess() : pid(-1), writeFd(-1), readFd(-1), clientFd(-1), bodyBytesSent(0), finished(false) {}
};

bool startCgi(const HttpRequesr &request, const std::string &scriptPath,
              const std::string &interpreter, CgiProcess &proc);

#endif
