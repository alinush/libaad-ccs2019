#include <iostream>
#include <aad/Library.h>
#include <aad/PublicParameters.h>
#include <aad/EllipticCurves.h>

using namespace std;
using namespace libaad;

int main(int argc, char *argv[])
{
    libaad::initialize(nullptr, 0);

    if(argc < 5) {
        cout << "Usage: " << argv[0] << " <trapdoor-file> <out-file> <start-incl> <end-excl>" << endl;
        cout << endl;
        cout << "Reads 's' and 'tau' from <trapdoor-file> and outputs q-PKE parameters (g_1^{s_i}, g_1^{tau s^i}, g_2^{s^i}) for i \\in [<start-incl>, <end-excl>) to <out-file>." << endl;
        return 1;
    }

    string inFile(argv[1]);
    string outFile(argv[2]);
    size_t start = static_cast<size_t>(std::stoi(argv[3])),
        end = static_cast<size_t>(std::stoi(argv[4]));

    ifstream fin(inFile);

    if(fin.fail()) {
        throw std::runtime_error("Could not read trapdoors input file");
    }

    Fr s, tau;
    fin >> s;
    fin >> tau;

    PublicParameters::generate(start, end, s, tau, outFile, true);


    loginfo << "All done!" << endl;

    return 0;
}
