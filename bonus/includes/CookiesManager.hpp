#ifndef COOKIESMANAGER_HPP
# define COOKIESMANAGER_HPP
#include "Cookie.hpp"
#include "map"
#include <sstream>
#include <cstdlib> 
#include <ctime> 
#include <iostream>


class CookiesManager
{
private:
    std::map<std::string, Cookie> _cookies;
public:
    CookiesManager();
    CookiesManager(const CookiesManager &other);
    CookiesManager& operator=(const CookiesManager &other);
    ~CookiesManager();
    Cookie *existCookie(std::string hash);
    std::string createCookie(std::string name);
    bool expired(std::map<std::string, Cookie>::iterator it);
    bool isValidSesion(std::string hash);
    std::string getUser(Cookie cookie);
};
unsigned long djb2Hash(const std::string &str);
#endif