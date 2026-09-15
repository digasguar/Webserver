#ifndef COOKIESMANAGER
# define COOKIESMANAGER
#include "Cookie.hpp"

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

    std::string getUser(Cookie cookie);
};
static unsigned long djb2Hash(const std::string &str);
#endif