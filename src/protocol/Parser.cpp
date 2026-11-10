#include "protocol/Parser.hpp"


static	void	skipSpace(const std::string &line, unsigned long *i)
{
	while (line[*i] == ' ')
		(*i)++;
}
static bool	skipPrefix(const std::string &line, unsigned long *icommand)
{
	if (line[*icommand] == ':')
		{
			*icommand = line.find(' ', *icommand);
			if (*icommand == std::string::npos)
				return false ;
			skipSpace(line, icommand);
		}
	return true ;
}
namespace irc
{
	ParseResult	parseCommand(const std::string &line, ParsedCommand &out)
	{
		if (line.size() == 0)
			return ParseIgnore ;

		unsigned long icommand = 0;
		skipSpace(line, &icommand);
		if (!skipPrefix(line, &icommand))
			return ParseError ;
		unsigned long i = line.find(' ', icommand);
		
		if (i == std::string::npos)
		{
			if (icommand < line.size())
			{
				out.command = line.substr(icommand);
				return ParseSuccess ;
			}
			return ParseError ;
		}

		out.command = line.substr(icommand, i - icommand);
		skipSpace(line, &i);
		
		for (unsigned long end = line.find(' ', i); end != std::string::npos; end = line.find(' ', i))
		{
			out.params.push_back(line.substr(i, end - i));
			i = end + 1;
			skipSpace(line, &i);
			if (out.params.size() == 15)
				return ParseError;
			if (line[i] == ':')
				break ;
		}
		if (line[i] == ':') //trailing param, go until EOF
			i++;
		if (i < line.size() || line[i - 1] == ':')
			out.params.push_back(line.substr(i));
		return (ParseSuccess);
	}
}
