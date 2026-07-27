#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP
#include <string>

struct HttpRequesr
{
    std::string type; // tipo de peticioon
    std::string path; // que pide el cliente
    std::string version; // que vrsiond de http es
};


#endif