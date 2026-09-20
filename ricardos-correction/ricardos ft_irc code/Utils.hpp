#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <vector>

// Small text helpers used everywhere.

std::string					lower(std::string text);
std::string					upper(std::string text);
std::string					toString(size_t number);

// split("a,b,c", ',')  ->  ["a", "b", "c"]
std::vector<std::string>	split(const std::string &text, char separator);

// splitLine("KICK #chan bob :see you")  ->  ["KICK", "#chan", "bob", "see you"]
std::vector<std::string>	splitLine(const std::string &line);

#endif
