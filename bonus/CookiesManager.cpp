#include "includes/CookiesManager.hpp"

CookiesManager::CookiesManager(){}

CookiesManager::~CookiesManager(){}

CookiesManager::CookiesManager(const CookiesManager &other): _cookies(other._cookies){}

CookiesManager &CookiesManager::operator=(const CookiesManager &other)
{
    if (this != &other)
        this->_cookies = other._cookies;
    return (*this);
}

std::string CookiesManager::createCookie(std::string name)
{
    std::stringstream ss;
    ss << name << time(NULL) << rand();
    unsigned long h = djb2Hash(ss.str());
    std::stringstream hash;
    hash << h;
    this->_cookies.insert(std::make_pair(hash.str(), Cookie(name)));
    return (hash.str());
}

Cookie *CookiesManager::existCookie(std::string hash)
{
    std::map<std::string, Cookie>::iterator it = this->_cookies.find(hash);
    if (it != this->_cookies.end() && expired(it))
        return (NULL); 
    return (&it->second);
}

bool CookiesManager::expired(std::map<std::string, Cookie>::iterator it)
{
    if (it->second.isExpired())
    {
        this->_cookies.erase(it);
        return (false);
    }
    return (true);
}

std::string CookiesManager::getUser(Cookie cookie)
{
    return (cookie.getName());
}

static unsigned long djb2Hash(const std::string &str)
{
    unsigned long hash = 5381;
    for (size_t i = 0; i < str.size();i++)
        hash = ((hash << 5) + hash) + static_cast<unsigned char>(str[i]);
    return hash;
}