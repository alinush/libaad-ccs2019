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

/**
   Multiplies polynomial by x^n.
   Specify if n is positive or not with 'pos'
 */
template <typename FieldT>
void poly_scale(vector<FieldT> &a, uint_64 n, bool pos) {
    if (pos) {
        vector<FieldT> r (a.size()+n, 0);
        for (uint_64 i = n; i < r.size(); i++) {
            r[i] = a[i-n];
        }
        a = r;
    } else {
        vector<FieldT> r (a.size()-n, 0);
        for (uint_64 i = 0; i < r.size(); i++) {
            r[i] = a[i+n];
        }
        a = r;
    }
}


/**
   Multiplies polynomial by a scalar 'p'
 */
template <typename FieldT>
void poly_scalar_mul(vector<FieldT> &a, int p) {
    for (uint_64 i = 0; i < a.size(); i++) {
        a[i] *= p;
    }
}


/**
   Extend polynomial to degree 'd'
 */
template<typename FieldT>
void poly_extend(vector<FieldT> &a, uint_64 d) {
    vector<FieldT> r (d, 0);
    for (uint_64 i = 0; i < a.size(); i++) {
        r[i] = a[i];
    }
    a = r;
}

/**
   Computes reciprocal of polynomial.
   As defined in Iowa State paper
 */
template<typename FieldT>
void poly_recip(vector<FieldT> &r, vector<FieldT> &p) {
    uint_64 k = p.size();

    if (k == 1) {
        r = {p[0].inverse()};
        return;
    }

    vector<FieldT> f (k/2);
    for (uint_64 i = p.size()-k/2, j = 0; i < p.size(); i++, j++) {
        f[j] = p[i];
    }

    vector<FieldT> q;
    poly_recip(q, f);
    vector<FieldT> r1 (q);
    poly_scalar_mul(r1, 2);
    poly_scale(r1, (3*k)/2-2, true);
    _polynomial_subtraction(r, r1, poly_mult(p, poly_mult(q, q)));
    poly_scale(r, k-2, false);
}

/**
   Computes quotient and remainder of division of polynomials.
   As defined in Iowa State paper, uses poly_recip
 */
template<typename FieldT>
void poly_divide_recip(vector<FieldT> &q, vector<FieldT> &r, vector<FieldT> &u, vector<FieldT> &v) {
    if (v.size() > u.size()) {
        q = {0};
        r = u;
        return;
    }

    uint_64 m = u.size()-1, n = v.size()-1;

    uint_64 nd = (uint_64)round(pow(2, ceil(log(n+1)/log(2)))) - 1 - n;
    vector<FieldT> ue = u, ve = v, s;
    poly_scale(ue, nd, true);
    poly_scale(ve, nd, true);

    uint_64 me = m + nd, ne = n + nd;

    poly_recip(s, ve);
    _polynomial_multiplication(q, ue, s);
    poly_scale(q, 2*ne, false);

    if (me > 2*ne) {
        vector<FieldT> r2 = {1}, r4, q2, rm2;

        poly_scale(r2, 2*ne, true);
        r4 = poly_mult(ue, poly_subtract(r2, poly_mult(s, ve)));
        poly_scale(r4, 2*ne, false);
        poly_divide_recip(q2, rm2, r4, ve);
        
        _polynomial_addition(q, q, q2);
    }

    _polynomial_subtraction(r, u, poly_mult(v, q));
}

/**
   Computes quotient and remainder of division of polynomials.
   This function uses Newton iteration.
 */
template<typename FieldT>
void poly_divide_rev(vector<FieldT> &q, vector<FieldT> &r, vector<FieldT> &a, vector<FieldT> &b) {
    if (b.size() > a.size()) {
        q = {0};
        r = a;
        return;
    }

    uint_64 da = a.size()-1, db = b.size()-1;
    uint_64 m = da - db;

    _reverse(a, a.size());
    _reverse(b, b.size());

    vector<FieldT> g0, g1;
    g0 = {b[0].inverse()};

    vector<FieldT> r2, r3;
    int l = max((int)ceil(log(m+1)/log(2)), 1);
    for (int i = 1; i <= l; i++) {
        _polynomial_multiplication(r2, g0, g0);
        _polynomial_multiplication(r3, b, r2);
        poly_scalar_mul(g0, 2);
        _polynomial_subtraction(g1, g0, r3);

        uint_64 sz = (1ull << i);
        if (i != l) g1.resize(sz);
        else g1.resize(m+1);

        g0 = g1;
    }

    _polynomial_multiplication(q, a, g1);
    q.resize(m+1);

    _reverse(q, q.size());
    _reverse(a, a.size());
    _reverse(b, b.size());

    vector<FieldT> r4;
    _polynomial_multiplication(r4, b, q);
    _polynomial_subtraction(r, a, r4);
}
