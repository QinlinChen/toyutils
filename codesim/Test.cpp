#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "CharReader.h"
#include "Fingerprint.h"

using namespace std;

string randomString(int length)
{
    string s(length, '\0');
    for (auto &c : s)
        c = 33 + (char)(rand() % (126 - 33));
    return s;
}

void densityTest()
{
    constexpr int textlen = 10000000;
    constexpr int k = 50;
    constexpr int t = 149;
    constexpr int w = t - k + 1;

    istringstream iss(randomString(textlen));
    CharReader reader(iss);

    auto fingerprint = Fingerprint::winnow(reader, k, t);
    double density = (double)fingerprint.size() / (textlen - k + 1);

    cout << "fingerprint size: " << fingerprint.size() << endl;
    cout << "density: " << density << endl;
    cout << "expected density: " << 2.0 / (w + 1) << endl;
}
