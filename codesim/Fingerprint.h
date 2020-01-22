#pragma once
#include <vector>
#include <cstdint>

#include "CharReader.h"

class Fingerprint
{
public:
    Fingerprint();
    ~Fingerprint();

    static std::vector<uint64_t>
    winnow(CharReader &charReader, int noiseThreshold, int guaranteeLength);

    static std::vector<uint64_t>
    sharedFingerprint(std::vector<uint64_t> &fingerprint1,
                      std::vector<uint64_t> &fingerprint2);
};

