#ifndef CONFIGLOOKUP_HPP
# define CONFIGLOOKUP_HPP
#include <string>
#include "ConfigTypes.hpp"

// busca, dentro de un ServerConfig, la location cuyo path es el prefijo mas largo
// que matchea el path pedido por el cliente. Devuelve NULL si ninguna location matchea.
const LocationConfig *findLocation(const ServerConfig &server, const std::string &path);

#endif
