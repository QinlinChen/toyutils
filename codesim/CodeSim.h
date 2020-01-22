#pragma once
#include <string>
#include <cstdint>
#include <vector>

class CodeSim
{
public:
    CodeSim(int noiseThreshold = 15, int guaranteeLength = 20, int verbose = 1);
    ~CodeSim();

    double similarityForCpp(const std::string &file1,
                            const std::string &file2);

private:
    std::vector<uint64_t> fingerprintForCpp(const std::string &file);

    int _noiseThreshold;
    int _guaranteeLength;
    int _verbose;
};

