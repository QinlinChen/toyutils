#include <fstream>
#include <iostream>

#include "CodeSim.h"
#include "CharReader.h"
#include "Fingerprint.h"


CodeSim::CodeSim(int noiseThreshold, int guaranteeLength, int verbose)
    : _noiseThreshold(noiseThreshold), _guaranteeLength(guaranteeLength),
    _verbose(verbose)
{
}

CodeSim::~CodeSim()
{
}

double CodeSim::similarityForCpp(const std::string &file1,
                                 const std::string &file2)
{
    auto fingerprint1 = fingerprintForCpp(file1);
    auto fingerprint2 = fingerprintForCpp(file2);
    if (fingerprint1.size() == 0 || fingerprint2.size() == 0)
        return 0.0;

    auto shared = Fingerprint::sharedFingerprint(fingerprint1, fingerprint2);
    double ratio1 = (double)shared.size() / fingerprint1.size();
    double ratio2 = (double)shared.size() / fingerprint2.size();

    if (_verbose) {
        std::cout << "shared fingerprints: " << shared.size() << std::endl;
        std::cout << file1 << ": " << ratio1 << ", "
                  << file2 << ": " << ratio2 << std::endl;
    }
    return (ratio1 + ratio2) / 2;
}

std::vector<uint64_t> CodeSim::fingerprintForCpp(const std::string &file)
{
    std::ifstream fin(file);
    if (!fin) {
        std::cerr << "Cannot open file: " << file << std::endl;
        exit(EXIT_FAILURE);
    }

    CppCharReader reader(fin);
    return Fingerprint::winnow(reader, _noiseThreshold, _guaranteeLength);
}
