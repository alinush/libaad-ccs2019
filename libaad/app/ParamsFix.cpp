#include <iostream>
#include <aad/Library.h>
#include <aad/PublicParameters.h>
#include <aad/EllipticCurves.h>

using namespace std;
using namespace libaad;

int main(int argc, char *argv[])
{
    libaad::initialize(nullptr, 0);
    
    if(argc < 2) {
        cout << "Usage: " << argv[0] << " <trapdoor-in-file>" << endl;
        cout << endl;
        cout << "Reads 's' and 'tau' and 'q' from <trapdoor-in-file>, computes g2^{tau} and rewrites the (full) parameters back to <trapdoor-in-file>" << endl;
        return 1;
    }

    string inFile(argv[1]);

    PublicParameters::regenerateTrapdoors(inFile);

    loginfo << "All done!" << endl;

    return 0;
}
