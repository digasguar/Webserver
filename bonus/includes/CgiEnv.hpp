#ifndef CGIENV_HPP
# define CGIENV_HPP

#include <string>
#include <vector>
#include "HttpRequest.hpp"

std::vector<std::string> buildCgiEnv(const HttpRequesr &request, const std::string &scriptPath);

#endif
