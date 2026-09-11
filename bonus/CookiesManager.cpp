#include "includes/CookiesManager.hpp"

CookiesManager::CookiesManager(){}

CookiesManager::~CookiesManager(){}

CookiesManager::CookiesManager(const CookiesManager &other){}

CookiesManager &CookiesManager::operator=(const CookiesManager &other){}

void CookiesManager::createCookie(std::string name)
{
    std::stringstream ss;
    ss << name << time(NULL) << rand();
    unsigned long h = djb2Hash(ss.str());
    std::stringstream hash;
    hash << h;
    this->_cookies.insert(std::make_pair(hash.str(), Cookie(name)));
}

Cookie CookiesManager::existCookie(std::string hash)
{
    std::map<std::string, Cookie>::iterator it = this->_cookies.find(hash);
    if (it != this->_cookies.end() && expired(it))
        //return (nullptr); 
    return (it->second);
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