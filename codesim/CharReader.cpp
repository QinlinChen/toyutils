#include <regex>
#include <iostream>

#include "CharReader.h"


CharReader::CharReader(std::istream &is)
    : _is(is)
{
}

CharReader::~CharReader()
{
}

bool CharReader::next(char *retChar)
{
    char ch;
    if (!(_is >> ch))
        return false;
    
    if (retChar)
        *retChar = ch;
    return true;
}

CppCharReader::CppCharReader(std::istream &is)
    : CharReader(is)
{
    std::regex re("[_a-zA-Z][_0-9a-zA-Z]*");
    std::string line;
    std::ostringstream oss;
    while (std::getline(is, line)) {
        oss << regex_replace(line, re, "v") << std::endl;
        // std::cout << regex_replace(line, re, "v") << std::endl;
    }
    _iss = std::istringstream(oss.str());
}

CppCharReader::~CppCharReader()
{
}

bool CppCharReader::next(char * retChar)
{
    char ch;
    if (!(_iss >> ch))
        return false;

    if (retChar)
        *retChar = ch;
    return true;
}
