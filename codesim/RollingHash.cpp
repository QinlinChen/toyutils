#include <cstring>
#include <iostream>

#include "RollingHash.h"


RollingHash::RollingHash(CharReader &charReader, int gramSize)
    : _charReader(charReader), _gramSize(gramSize),
    _buf(nullptr), _r(0), _hash(0), _powBase(1)
{
    if (gramSize <= 0) {
        std::cerr << "Invalid value: gramSize should be positive." << std::endl;
        exit(1);
    }

    _buf = new char[gramSize];
    memset(_buf, 0, gramSize * sizeof(_buf[0]));
    for (int i = 0; i < gramSize; ++i)
        _powBase *= _base;
    for (int i = 0; i < gramSize - 1; ++i)
        if (!next(nullptr))
            return;
}

RollingHash::~RollingHash()
{
    delete _buf;
}

bool RollingHash::next(uint64_t *retHash)
{
    char nextChar;
    if (!(_charReader.next(&nextChar)))
        return false;

    _hash = ((_hash - (uint8_t)_buf[_r] * _powBase) + (uint8_t)nextChar) * _base;
    _buf[_r] = nextChar;
    _r = (_r + 1) % _gramSize;

    if (retHash)
        *retHash = _hash;
    return true;
}
