#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>

struct ParsedCommand
{
	std::string					prefix;
	std::string					command;
	std::vector<std::string>	params;
	std::string					trailing;
	bool						hasTrailing;

	ParsedCommand();
};

class Parser
{
	public:
		ParsedCommand	parse(const std::string &line) const;
};

#endif