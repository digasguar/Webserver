#ifndef COOKIE_HPP
# define COOKIE_HPP
# include "Librari.hpp"
# include <functional>
# define DATAEXPIRES 200

class Cookie
{
private:
    std::string _name;
    time_t _dataExpire;
public:
    Cookie(std::string name);
    Cookie(const Cookie &other);
    Cookie& operator=(const Cookie &other);
    ~Cookie();
    std::string getName() const;
    time_t getDataExpire() const;
    bool isExpired();
};
static unsigned long djb2Hash(std::string &string);

#endif