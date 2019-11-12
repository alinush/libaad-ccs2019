#include <iostream>
#include <ctime>
#include <vector>
#include <sstream>

#include <aad/NtlLib.h>
#include <aad/PolyOps.h>
#include <aad/PolyInterpolation.h>
#include <aad/Library.h>

#include <xassert/XAssert.h>
#include <xutils/Log.h>
#include <xutils/Timer.h>

using namespace NTL;
using namespace std;
using namespace libaad;

int main(int argc, char * argv[])
{
    libaad::initialize(nullptr, 0);

    loginfo << "Initial NTL numThreads: " << NTL::AvailableThreads() << endl;

    size_t startSz = 1024;
    size_t numIters = 2;
    if(argc > 1)
        startSz = static_cast<size_t>(std::stoi(argv[1]));

    if(argc > 2) {
        long numThreads = std::stoi(argv[2]);
        if(numThreads > static_cast<int>(getNumCores())) {
            logerror << "Number of cores for libntl (" << numThreads << ") cannot be bigger than # of cores on machine, which is " << getNumCores() << endl;
            return 1;
        }
        NTL::SetNumThreads(numThreads);
        loginfo << "Changed NTL NumThreads to " << numThreads << endl;
        loginfo << "NTL pool active: " << NTL::GetThreadPool()->active() << endl;
    }

    vector<Fr> aad1;
    for(size_t sz = startSz; sz < 1024*1024*512; sz *= 2) {
        loginfo << "Benching interpolating degree " << sz << endl;
        aad1.reserve(sz);
        while(aad1.size() < sz) {
            aad1.push_back(Fr::random_element());
        }
        logdbg << "Picked random polynomials!" << endl;

        vector<Fr> a, b;

        bool skipped = false;
        if(sz <= 1024*32) {
            AveragingTimer t1("poly_from_roots (libfqfft)  ");
            for(size_t i = 0; i < numIters; i++) {
                t1.startLap();
                poly_from_roots(a, aad1);
                t1.endLap();
            }
            loginfo << t1 << endl;
        } else {
            skipped = true;
            loginfo << "poly_from_roots (libfqfft) skipped, too slow." << endl;
        }
        
        AveragingTimer t2("poly_from_roots_ntl (libntl)");
        for(size_t i = 0; i < numIters; i++) {
            t2.startLap();
            poly_from_roots_ntl(b, aad1);
            t2.endLap();
        }
        loginfo << t2 << endl;
        loginfo << endl;

        if(!skipped) {
            testAssertTrue(a == b);
        }
    }
    
    return 0;
}
