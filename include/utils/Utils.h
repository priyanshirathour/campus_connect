#pragma once
#include <string>

std::string nowTimestamp();
std::string makeId(const std::string& prefix);
std::string toLowerCopy(std::string value);
std::string inputLine(const std::string& prompt);
std::string inputSecret(const std::string& prompt);
int inputInt(const std::string& prompt, int minimum, int maximum);
