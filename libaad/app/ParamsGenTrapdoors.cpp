#include <iostream>
#include <aad/Library.h>
#include <aad/PublicParameters.h>
#include <aad/EllipticCurves.h>

using namespace std;
using namespace libaad;

int main(int argc, char *argv[])
{
    libaad::initialize(nullptr, 0);
    
    if(argc < 3) {
        cout << "Usage: " << argv[0] << " <trapdoor-file> <q>" << endl;
        cout << endl;
        cout << "Generates 's' and 'tau' and writes them and 'q' to <trapdoor-file>" << endl;
        return 1;
    }

    string outFile(argv[1]);
    size_t q = static_cast<size_t>(std::stoi(argv[2]));

    ofstream fout(outFile);

    if(fout.fail()) {
        throw std::runtime_error("Could not open trapdoor output file");
    }

    Fr s = Fr::random_element(), tau = Fr::random_element();
    fout << s << endl;
    fout << tau << endl;
    fout << q << endl;
    fout << tau * G2::one() << endl;

    loginfo << "All done!" << endl;

    return 0;
}
