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
        cout << "Reads 's', 'tau', 'q' and 'g2^{tau}' from <trapdoor-in-file> and then reads the parameters from <trapdoor-in-file>-<i> for i = 0, 1, ..." << endl;
        return 1;
    }

    string inFile(argv[1]);

    PublicParameters pp(inFile, -1, true, true);

    loginfo << "All done!" << endl;

    return 0;
}
