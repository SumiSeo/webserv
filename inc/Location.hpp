#ifndef Location_HPP
# define Location_HPP

# include "iostream"
class Location
{
  private:
	std::string key;
	std::string value;

  protected:
  public:
	Location(std::string key, std::string value);
	Location(const Location &original);
	Location &operator=(const Location &original);
	~Location();
};

#endif