
#include "Location.hpp"

Location::Location(){};
Location::Location(std::string key, std::string value)
{
	this->key = key;
	this->value = value;
}
Location::Location(const Location &original)
{
	if (this != &original)
	{
		*this = original;
	}
};
Location &Location::operator=(const Location &original)
{
	if (this != &original)
	{
		*this = original;
	}
	return (*this);
};
Location::~Location(){};
