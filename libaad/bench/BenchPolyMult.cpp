#include <aad/Library.h>
#include <aad/PolyOps.h>
#include <aad/EllipticCurves.h>

#include <vector>
#include <cmath>
#include <iostream>
#include <ctime>
#include <fstream>

#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>
#include <libff/common/double.hpp>

#include <xutils/Log.h>
#include <xutils/Timer.h>
#include <xassert/XAssert.h>

using namespace std;
using namespace libfqfft;
using namespace libaad;

void assertPolyEqual(const std::vector<Fr>& p1, const std::vector<Fr>& p2) {
    std::vector<Fr> res;
    _polynomial_subtraction(res, p1, p2);
    if (_is_zero(res) == false) {
        assertFail("The two multiplication functions returned different products");
    }
}

int main() {
    libaad::initialize(nullptr, 0);
    for (size_t i = 1; i <= 1024*1024; i *= 2) {
        vector<Fr> a(i), b(i);
        vector<Fr> p1, p2, p3, res;

        double count = 2.0;

        AveragingTimer c1("FFT"), c3("NTL"), c2("N^2");
        for (int rep = 0; rep < count; rep++) {
            for (unsigned long int i = 0; i < a.size(); i++)
                a[i] = Fr::random_element();

            for (unsigned long int i = 0; i < b.size(); i++)
                b[i] = Fr::random_element();

            c1.startLap();
            _polynomial_multiplication(p1, a, b);
            c1.endLap();

            if(i < 8192) {
                c2.startLap();
                _polynomial_multiplication_naive(p2, a, b);
                c2.endLap();
            }
            
            c3.startLap();
            poly_multiply_ntl(p3, a, b);
            c3.endLap();
    
            assertPolyEqual(p1, p2);
            assertPolyEqual(p1, p3);

            p1.clear();
            p2.clear();
            p3.clear();
        }
        logperf << "a.size() = " << a.size() << ", b.size() = " << b.size() << ", iters = " << count
                << endl;
        logperf << c1 << endl;
        logperf << c3 << endl;
        logperf << c2 << endl;
        logperf << endl;
    }
    return 0;
}
