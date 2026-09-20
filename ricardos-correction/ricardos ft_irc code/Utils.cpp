#include "Utils.hpp"
#include <cctype>
#include <sstream>

std::string	lower(std::string text)
{
	for (size_t i = 0; i < text.size(); i++)
		text[i] = std::tolower(static_cast<unsigned char>(text[i]));
	return text;
}

std::string	upper(std::string text)
{
	for (size_t i = 0; i < text.size(); i++)
		text[i] = std::toupper(static_cast<unsigned char>(text[i]));
	return text;
}

std::string	toString(size_t number)
{
	std::ostringstream out;
	out << number;
	return out.str();
}

std::vector<std::string>	split(const std::string &text, char separator)
{
	std::vector<std::string>	parts;
	std::istringstream			in(text);
	std::string					part;

	while (std::getline(in, part, separator))
		if (!part.empty())
			parts.push_back(part);
	return parts;
}

// An IRC line is a list of words separated by spaces.
// A word that starts with ':' means "everything until the end of the line
// is one single argument" (this is how messages with spaces are sent).
std::vector<std::string>	splitLine(const std::string &line)
{
	std::vector<std::string>	args;
	size_t						i = 0;

	while (i < line.size())
	{
		if (line[i] == ' ')
		{
			i++;
			continue;
		}
		if (line[i] == ':' && !args.empty())
		{
			args.push_back(line.substr(i + 1));
			break;
		}
		size_t end = line.find(' ', i);
		if (end == std::string::npos)
			end = line.size();
		args.push_back(line.substr(i, end - i));
		i = end;
	}
	// a line may start with a ":prefix" word, we do not need it
	if (!args.empty() && args[0][0] == ':')
		args.erase(args.begin());
	return args;
}
