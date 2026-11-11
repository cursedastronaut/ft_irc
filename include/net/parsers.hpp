#ifndef PARSERS_HPP
# define PARSERS_HPP

# include <algorithm>
# include <iostream>
# include <climits>
# include <sstream>
# include <string>

# include "settings.hpp"

# ifndef STRINGIFY
#  define STRINGIFY(x) #x
# endif

# ifndef TOSTRING
#  define TOSTRING(x) STRINGIFY(x)
# endif

//	Exception
class ParserPortEmptyPort : public std::exception {
	virtual const char	*what() const throw();
};

class ParserPortIvalideFormat : public std::exception {
	virtual const char	*what() const throw();
};

class ParserPortOverFlow : public std::exception {
	virtual const char	*what() const throw();
};

class ParserPortOutOfRange : public std::exception {
	virtual const char	*what() const throw();
};

class ParserPSWEmpty : public std::exception {
	virtual const char	*what() const throw();
};

int				port_parser(const char *str_port);
std::string		psw_parser(const char *str_psw);

#endif	//	PARSER_HPP
