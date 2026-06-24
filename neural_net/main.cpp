#include "interface.h"

#include <exception>
#include <iostream>
#include <string>

using namespace std;

static void PrintUsage(const char* programName) {
    cerr << "Usage: " << programName << " --image <path> --weights <path>" << endl;
}

int main(int argc, char* argv[]) {
    string imagePath;
    string weightsPath = "net_weights.txt";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "--image" && i + 1 < argc) {
            imagePath = argv[++i];
        } else if (arg == "--weights" && i + 1 < argc) {
            weightsPath = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            PrintUsage(argv[0]);
            return 0;
        } else {
            cerr << "Unknown or incomplete argument: " << arg << endl;
            PrintUsage(argv[0]);
            return 2;
        }
    }

    if (imagePath.empty()) {
        cerr << "Missing required argument: --image" << endl;
        PrintUsage(argv[0]);
        return 2;
    }

    try {
        NetInterface(imagePath, weightsPath);
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return 1;
    }

    return 0;
}
