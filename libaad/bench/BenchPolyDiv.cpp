#include <aad/Utils.h>
#include <aad/Library.h>
#include <aad/EllipticCurves.h>
#include <aad/PolyOps.h>
#include <aad/unnecessary/PolyDivision.h>

#include <cstdlib>
#include <vector>
#include <cmath>
#include <iostream>

#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>
#include <libff/common/double.hpp>

#include <xassert/XAssert.h>
#include <xutils/Log.h>
#include <xutils/Timer.h>

using namespace std;
using namespace libfqfft;
using libaad::Fr;

int main(int argc, char * argv[])
{
    libaad::initialize(nullptr, 0);

    srand(42);

    size_t max = 1024*1024;
    if(argc > 1) {
        max = static_cast<size_t>(std::stoi(argv[1]));
    }

    vector<Fr> a, b;

    for(size_t i = 32768; i < max; i *= 2) {
        size_t n1 = 2*i - 100;
        size_t n2 = i;

        a.reserve(n1);
        b.reserve(n2);

        loginfo << "Picking random polynomials (deg " << n1 << " and deg " << n2 << ")..." << endl;
        while(a.size() < n1) {
            a.push_back(Fr::random_element());
        }

        while(b.size() < n2) {
            b.push_back(Fr::random_element());
        }

        loginfo << " * Done" << endl;

        vector<Fr> q_recp, r_recp, 
                q_rev, r_rev,
                q_ntl, r_ntl;
        
        {
            logperf << std::flush;
            ScopedTimer<std::chrono::seconds> t(std::cout, "Reciprocal-based division took ", " seconds\n");
            poly_divide_recip(q_recp, r_recp, a, b);
        }

        {
            logperf << std::flush;
            ScopedTimer<std::chrono::seconds> t(std::cout, "Reversal-based division took   ", " seconds\n");
            poly_divide_rev(q_rev, r_rev, a, b);
        }
        
        {
            logperf << std::flush;
            ScopedTimer<std::chrono::seconds> t(std::cout, "libntl division took           ", " seconds\n");
            poly_divide_ntl(q_ntl, r_ntl, a, b);
        }
        
        if(n1 < 1024*8) {
            vector<Fr> q, r;
            logperf << std::flush;
            {
                ScopedTimer<std::chrono::seconds> t(std::cout, "Naive division took            ", " seconds\n");
                _polynomial_division(q, r, a, b);
            }
            testAssertTrue(poly_equal(r_recp, r));
            testAssertTrue(poly_equal(q_recp, q));
            testAssertTrue(poly_equal(r_rev, r));
            testAssertTrue(poly_equal(q_rev, q));
            testAssertTrue(poly_equal(r_ntl, r));
            testAssertTrue(poly_equal(q_ntl, q));
        }

        testAssertTrue(poly_equal(q_recp, q_rev));
        testAssertTrue(poly_equal(r_recp, r_rev));
        testAssertTrue(poly_equal(q_ntl, q_rev));
        testAssertTrue(poly_equal(r_ntl, r_rev));

        loginfo << endl;
    }

    return 0;
}


