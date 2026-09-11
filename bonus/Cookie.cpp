#include "includes/Cookie.hpp"

Cookie::Cookie(std::string name): _name(name){this->_dataExpire = time(NULL);}

Cookie::~Cookie(){}

Cookie::Cookie(const Cookie &other):_name(other.getName()), _dataExpire(other.getDataExpire()){}

Cookie& Cookie::operator=(const Cookie &other)
{
    if (this != &other)
    {
        this->_name = other.getName();
        this->_dataExpire = other.getDataExpire();
    }
    return (*this);
}
std::string Cookie::getName() const {return(this->_name);}

time_t Cookie::getDataExpire() const {return(this->_dataExpire);}

bool Cookie::isExpired()
{
    time_t now = time(NULL); 
    if (now - this->_dataExpire  >= DATAEXPIRES)
        return (true);
    return (false);
}
