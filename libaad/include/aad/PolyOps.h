#pragma once

#include <vector>
#include <algorithm>
#include <cmath>
#include <iostream>

#include <aad/Utils.h>
#include <aad/NtlLib.h>
#include <aad/PolyCommit.h>

#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>

using namespace std;
using namespace libfqfft;
using namespace NTL;

typedef long unsigned int uint_64;

template <typename FieldT>
void poly_divide_ntl(vector<FieldT> &q, vector<FieldT> &r, const vector<FieldT> &u, const vector<FieldT> &v) {
    ZZ_pX polyQ, polyR, polyU, polyV;

    libaad::conv_fr_zp(u, polyU);
    libaad::conv_fr_zp(v, polyV);
    polyQ = ZZ_pX(INIT_MONO, 0);
    polyR = ZZ_pX(INIT_MONO, 0);

    DivRem(polyQ, polyR, polyU, polyV);

    libaad::convNtlToLibff(polyQ, q);
    libaad::convNtlToLibff(polyR, r);
}

/**
   O(n^2) polynomial multiplication
 */
template<typename FieldT>
void _polynomial_multiplication_naive(vector<FieldT> &c, const vector<FieldT> &b, const vector<FieldT> &a) {
    c.resize(a.size() + b.size() - 1, 0);
    for (size_t i = 0; i < a.size(); i++)
        for (size_t j = 0; j < b.size(); j++)
            c[i+j] += a[i] * b[j];
}

template <typename FieldT>
void poly_multiply_ntl(vector<FieldT> &r, const vector<FieldT> &u, const vector<FieldT> &v) {

    ZZ_pX polyR, polyU, polyV;

    libaad::conv_fr_zp(u, polyU);
    libaad::conv_fr_zp(v, polyV);
    polyR = ZZ_pX(INIT_MONO, 0);

    mul(polyR, polyU, polyV);

    libaad::convNtlToLibff(polyR, r);
}

// NOTE: Faster than libntl's multiplication
template<typename FieldT>
void poly_multiply(vector<FieldT> &c, const vector<FieldT> &b, const vector<FieldT> &a) {
    if(a.size() + b.size() <= 128) {
        return _polynomial_multiplication_naive(c, b, a);
    } else {
        return _polynomial_multiplication(c, b, a);
    }
}

template<typename FieldT>
inline const vector<FieldT> poly_mult(const vector<FieldT> &a, const vector<FieldT> &b) {
       vector<FieldT> ret;
       _polynomial_multiplication(ret, a, b);
       return ret;
}

template<typename FieldT>
inline const vector<FieldT> poly_subtract(const vector<FieldT> &a, const vector<FieldT> &b) {
       vector<FieldT> ret;
       _polynomial_subtraction(ret, a, b);
       return ret;
}

template<typename FieldT>
void eea_ntl(const vector<FieldT> &x, const vector<FieldT> &y, vector<FieldT> &a, vector<FieldT> &b) {
    ZZ_pX polyA, polyB, polyS, polyT, polyD;

    libaad::conv_fr_zp(x, polyA);
    libaad::conv_fr_zp(y, polyB);
    polyS = ZZ_pX(INIT_MONO, 0);
    polyT = ZZ_pX(INIT_MONO, 0);
    polyD = ZZ_pX(INIT_MONO, 0);

    XGCD(polyD, polyS, polyT, polyA, polyB);

    libaad::convNtlToLibff(polyS, a);
    libaad::convNtlToLibff(polyT, b);
}

template<class T>
bool poly_equal(const std::vector<T>& a, const std::vector<T>& b) {
    std::vector<T> res;
    _polynomial_subtraction(res, a, b);
    
    return res.size() == 0;
}
