#ifndef __Structs_HPP
# define __Structs_HPP

# include "iostream"
# include <fstream>
# include <iostream>
# include <sstream>
# include <string>
# include <vector>

using std::ifstream;
using std::string;
using std::stringstream;
using std::vector;

typedef vector<string> t_vecString;

class Structs
{
  private:
	/* data */
  protected:
	/* data */
  public:
	Structs(/* args*/);
	Structs(const Structs &original);
	Structs &operator=(const Structs &original);
	~Structs();
};

#endif