#include <iostream>
#include <ctime>
#include <vector>
#include <sstream>

#include <NTL/ZZ.h>
#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include <NTL/vec_ZZ_p.h>
#include <NTL/vector.h>

#include <aad/PolyOps.h>
#include <aad/PolyInterpolation.h>
#include <aad/Library.h>
#include <aad/NtlLib.h>
#include <aad/Utils.h>

#include <xutils/Timer.h>

using namespace NTL;
using namespace std;
using namespace libaad;

int main()
{
    libaad::initialize(nullptr, 0);

    size_t sz = 1024*128;

    printMemUsage("Before");

    std::vector<Fr> fr, fr2;

    loginfo << endl;
    loginfo << "Picking " << sz << " random Fr's... ";
    for (size_t i = 0; i < sz; i++) {
        fr.push_back(Fr::random_element());
    }
    cout << "done." << endl;
    fr2.resize(fr.size());

    ZZ_pX zp(INIT_MONO, (long)(fr.size()-1));

    ManualTimer t;
    t.restart();
    conv_fr_zp(fr, zp);
    auto mus = t.stop();
    logperf << sz << " Fr to ZZ_p conversions: " << mus.count() << " microsecs" << endl;
    
    t.restart();
    convNtlToLibff(zp, fr2); 
    mus = t.stop();
    logperf << sz << " ZZ_p to Fr conversions (w/ stringstream): " << mus.count() << " microsecs" << endl;
    
    loginfo << endl;
    printMemUsage("After");

    if(fr != fr2) {
        logerror << "Converting back from ZZ_p to Fr failed" << endl;
        return 1;
    }

    loginfo << "All done!" << endl;
    
    return 0;
}
