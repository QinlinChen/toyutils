#pragma once
#include <cstdint>

#include "CharReader.h"

class RollingHash
{
public:
    RollingHash(CharReader &charReader, int gramSize);
    ~RollingHash();

    bool next(uint64_t *retHash);

private:
    static const uint64_t _base = 1000000007ul;

    CharReader &_charReader;
    int _gramSize;

    char *_buf;
    int _r;

    uint64_t _hash;
    uint64_t _powBase;
};

