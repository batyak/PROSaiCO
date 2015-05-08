
#ifndef CONVERT_H
#define CONVERT_H

#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <typeinfo>

class Convert
{
public:
	template <typename T>
	static std::string T_to_string(T const &val) 
	{
		std::ostringstream ostr;
		ostr << val;

		return ostr.str();
	}
		
	template <typename T>
	static T string_to_T(std::string const &val) 
	{
		std::istringstream istr(val);
		T returnVal;
		if (!(istr >> returnVal)){
			std::cout << " error inducing value: " << val << std::endl;
			exitWithError1("CFG: Not a valid " + (std::string)typeid(T).name() + " received!  \n");
		}

		return returnVal;
	}
/*
	template <>
	static std::string string_to_T(std::string const &val)
	{
		return val;
	}
*/
	static void exitWithError1(const std::string &error) 
	{
		std::cout << error;
		std::cin.ignore();
		std::cin.get();

		exit(EXIT_FAILURE);
	}

};


#endif
