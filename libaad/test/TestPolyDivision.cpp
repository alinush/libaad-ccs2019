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

using namespace std;
using namespace libfqfft;
using libaad::Fr;

int main()
{
    libaad::initialize(nullptr, 0);

    vector<Fr> a (10000), b (5000);

    srand((unsigned int) time(0));

    for (unsigned long int i = 0; i < a.size(); i++) {
        a[i] = Fr::random_element();
    }

    for (unsigned long int i = 0; i < b.size(); i++) {
        b[i] = Fr::random_element();
    }

    vector<Fr> q, r, q1, r1, q2, r2;
    poly_divide_recip(q, r, a, b);
    poly_divide_rev(q1, r1, a, b);
    _polynomial_division(q2, r2, a, b);

    vector<Fr> res1, res2, res3;
    _polynomial_subtraction(res1, q1, q2);
    _polynomial_subtraction(res2, q, q2);
    _polynomial_subtraction(res3, r, r1);

    if (res1.size() != 0 || res2.size() != 0) {
        throw logic_error("Division failed: different quotient");
    }

    if (res3.size() != 0) {
        throw logic_error("Division failed: different remainder");
    }

    std::cout << "Division test passed!" << std::endl;

    return 0;
}


