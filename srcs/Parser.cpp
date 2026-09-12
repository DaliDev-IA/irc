#include "Parser.hpp"

ParsedCommand::ParsedCommand()
{
	hasTrailing = false;
}

ParsedCommand	Parser::parse(const std::string &line) const
{
	ParsedCommand result;
	std::string::size_type pos = 0;
	std::string::size_type start;
	std::string::size_type end;

	while (pos < line.size() && line[pos] == ' ')
		pos++;

	if (pos < line.size() && line[pos] == ':')
	{
		pos++;
		start = pos;

		while (pos < line.size() && line[pos] != ' ')
			pos++;

		result.prefix = line.substr(start, pos - start);

		while (pos < line.size() && line[pos] == ' ')
			pos++;
	}

	start = pos;

	while (pos < line.size() && line[pos] != ' ')
		pos++;

	result.command = line.substr(start, pos - start);

	while (pos < line.size())
	{
		while (pos < line.size() && line[pos] == ' ')
			pos++;

		if (pos >= line.size())
			break;

		if (line[pos] == ':')
		{
			pos++;
			result.trailing = line.substr(pos);
			result.hasTrailing = true;
			break;
		}

		start = pos;

		while (pos < line.size() && line[pos] != ' ')
			pos++;

		end = pos;
		result.params.push_back(line.substr(start, end - start));
	}

	return (result);
}