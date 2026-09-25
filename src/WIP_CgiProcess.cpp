#include "../includes/CgiProcess.hpp"
#include "../includes/CgiEnv.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <cstring>

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static std::string dirnameOf(const std::string &path)
{
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        return (".");
    return (path.substr(0, slash));
}

// una vez hecho chdir() a dirnameOf(scriptPath), execve() debe recibir SOLO el nombre del archivo
// no la ruta completa otra vez (si no, se busca ./carpeta/carpeta/archivo, que no existe)
static std::string basenameOf(const std::string &path)
{
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        return (path);
    return (path.substr(slash + 1));
}

bool startCgi(const HttpRequesr &request, const std::string &scriptPath,
              const std::string &interpreter, CgiProcess &proc)
{
    int inPipe[2];
    int outPipe[2];

    if (pipe(inPipe) < 0)
        return (false);
    if (pipe(outPipe) < 0)
    {
        close(inPipe[0]);
        close(inPipe[1]);
        return (false);
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        return (false);
    }

    if (pid == 0)
    {
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);

        close(inPipe[0]);
        close(inPipe[1]);
        close(outPipe[0]);
        close(outPipe[1]);

        if (chdir(dirnameOf(scriptPath).c_str()) != 0)
            _exit(1);

        std::vector<std::string> envVec = buildCgiEnv(request, scriptPath);

        std::vector<char *> argv;
        argv.push_back(const_cast<char *>(interpreter.c_str()));
        std::string scriptBasename = basenameOf(scriptPath);
        argv.push_back(const_cast<char *>(scriptBasename.c_str()));
        argv.push_back(NULL);

        std::vector<char *> envp;
        for (size_t i = 0; i < envVec.size(); ++i)
            envp.push_back(const_cast<char *>(envVec[i].c_str()));
        envp.push_back(NULL);

        execve(interpreter.c_str(), &argv[0], &envp[0]);
        _exit(1);
    }

    close(inPipe[0]);
    close(outPipe[1]);

    setNonBlocking(inPipe[1]);
    setNonBlocking(outPipe[0]);

    proc.pid = pid;
    proc.writeFd = inPipe[1];
    proc.readFd = outPipe[0];
    proc.bodyBytesSent = 0;
    proc.outputSoFar.clear();
    proc.finished = false;

    return (true);
}
