#ifndef CONFIGTOKENIZER_HPP
# define CONFIGTOKENIZER_HPP
#include <string>
#include <vector>

bool tokenizeConfigFile(const std::string &path, std::vector<std::string> &tokens);

#endif
