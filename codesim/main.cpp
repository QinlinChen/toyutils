#include <iostream>
#include <getopt.h>

#include "CodeSim.h"
#include "Test.h"

using namespace std;


void show_usage(const char *cmd, int status)
{
    cout << "Usage: " << cmd
         << " [-h, --help] [-v, --verbose] FILE1 FILE2" << endl;
    exit(status);
}

int main(int argc, char *argv[])
{
    int verbose = 0;

    /* Process args. */
    while (1) {
        static const char *optstring = "hv";
        static const struct option longopts[] = {
            {"help", no_argument, NULL, 'h'},
            {"verbose", no_argument, NULL, 'v'},
            {NULL, no_argument, NULL, 0}};

        int opt = getopt_long(argc, argv, optstring, longopts, NULL);
        if (opt == -1)
            break;

        switch (opt) {
        case 'v':
            verbose = 1;
            break;
        case 'h':
            show_usage(argv[0], EXIT_SUCCESS);
            break;
        default: /* 0, ?, etc. */
            show_usage(argv[0], EXIT_FAILURE);
        }
    }

    if (optind + 1 >= argc) {
        cerr << "Expect at least two files." << endl;
        show_usage(argv[0], EXIT_FAILURE);
    }

    string file1(argv[optind]), file2(argv[optind + 1]);
    CodeSim codesim(15, 20, verbose);
    cout << codesim.similarityForCpp(file1, file2) << endl;

    return 0;
}