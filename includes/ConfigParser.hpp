#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP
#include <string>
#include <vector>
#include "ConfigTypes.hpp"

struct TokenCursor
{
    const std::vector<std::string> &tokens;
    size_t pos;

    TokenCursor(const std::vector<std::string> &t) : tokens(t), pos(0) {}

    bool hasMore() const { return (pos < tokens.size()); }
    const std::string &peek() const { return (tokens[pos]); }
    const std::string &advance() { return (tokens[pos++]); }
};

// consume los tokens del ConfigTokenizer y construye el Config (vector<ServerConfig>)
// devuelve false y rellena errorMessage si hay cualquier error de sintaxis/semantica,
// nunca crashea ni deja el Config en un estado indefinido
bool parseConfig(const std::vector<std::string> &tokens, Config &config, std::string &errorMessage);

#endif
