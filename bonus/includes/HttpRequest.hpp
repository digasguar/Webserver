#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP
#include <string>
#include <iostream>
#include <map>

struct HttpRequesr
{
	std::string type; // tipo de peticioon
	std::string path; // que pide el cliente
	std::string version; // que vrsiond de http es
	std::string body;
	std::string query; //lo que va despues del ? en la request
	std::map<std::string, std::string> headers; //IMPORTANTE: las keys estan en minusculas, porque hay veces que la request puede venir en minusculas, y para no hacer un cristo con los ifs

	std::string pathInfo; // lo que sobra de la URL tras el script CGI: /cgi-bin/x.py/ESTO
	std::string remoteAddr; // IP del cliente (la rellena el servidor al aceptar la conexion)
	std::string serverPort; // puerto del servidor que atendio la peticion
};


#endif
