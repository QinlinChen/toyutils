#pragma once
#include <istream>
#include <sstream>


class CharReader
{
public:
    CharReader(std::istream &is);
    virtual ~CharReader();

    virtual bool next(char *retChar);

private:
    std::istream &_is;
};


class CppCharReader: public CharReader
{
public:
    CppCharReader(std::istream &is);
    ~CppCharReader();

    bool next(char *retChar) override;

public:
    std::istringstream _iss;
};