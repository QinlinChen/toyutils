#include <iostream>
#include <algorithm>
#include <cstdlib>

#include "Fingerprint.h"
#include "RollingHash.h"


Fingerprint::Fingerprint()
{
}

Fingerprint::~Fingerprint()
{
}

std::vector<uint64_t>
Fingerprint::winnow(CharReader &charReader, int noiseThreshold, int guaranteeLength)
{
    if (noiseThreshold <= 0) {
        std::cerr << "Invalid value: noiseThreshold should be positive." << std::endl;
        exit(1);
    }

    if (guaranteeLength <= 0) {
        std::cerr << "Invalid value: guaranteeLength should be positive." << std::endl;
        exit(1);
    }

    if (guaranteeLength < noiseThreshold) {
        std::cerr << "Invalid value: noiseThreshold should be"
            " no greater than guaranteeLength" << std::endl;
        exit(1);
    }

    int windowSize = guaranteeLength - noiseThreshold + 1;
    std::vector<uint64_t> hashBuf(windowSize, UINT64_MAX);
    int r = 0, minIndex = 0;

    RollingHash rollingHash(charReader, noiseThreshold);
    std::vector<uint64_t> result;

    while (rollingHash.next(&hashBuf[r])) {
        if (minIndex == r) {
            for (int i = (r - 1 + windowSize) % windowSize;
                 i != r; i = (i - 1 + windowSize) % windowSize)
                if (hashBuf[i] < hashBuf[minIndex])
                    minIndex = i;
            result.push_back(hashBuf[minIndex]);
        }
        else {
            if (hashBuf[r] < hashBuf[minIndex]) {
                minIndex = r;
                result.push_back(hashBuf[minIndex]);
            }
        }
        r = (r + 1) % windowSize;
    }

    return result;
}

std::vector<uint64_t>
Fingerprint::sharedFingerprint(std::vector<uint64_t> &fingerprint1,
                               std::vector<uint64_t> &fingerprint2)
{
    std::vector<uint64_t> result;

    std::sort(fingerprint1.begin(), fingerprint1.end());
    std::sort(fingerprint2.begin(), fingerprint2.end());
    std::set_intersection(fingerprint1.begin(), fingerprint1.end(),
                          fingerprint2.begin(), fingerprint2.end(),
                          std::back_inserter(result));
    return result;
}