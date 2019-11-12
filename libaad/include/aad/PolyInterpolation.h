#pragma once

#include <ctime>
#include <vector>
#include <stack>
#include <queue>
#include <iterator>
#include <algorithm>

#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>
#include <aad/PolyOps.h>
#include <aad/PolyCommit.h>
#include <aad/NtlLib.h>

using namespace libaad;
using namespace libfqfft;
using namespace std;

typedef long unsigned int uint_64;

// NOTE: In BenchNtlInterpolate.cpp, we benchmarked this against poly_from_roots_ntl and poly_from_roots_ntl is much faster
template <typename FieldT>
void poly_from_roots(vector<FieldT> &pol, const vector<FieldT> &un) {
    queue< vector<FieldT> > merge;
    for (auto el : un) merge.push({FieldT::zero() - el, 1});

    while (merge.size() > 1) {
        vector<FieldT> res, f, s;
        f = merge.front();
        merge.pop();
        s = merge.front();
        merge.pop();
        _polynomial_multiplication(res, f, s);
        merge.push(res);
    }

    pol = merge.front();
    merge.pop();
}

template<typename FieldT>
void poly_from_roots_ntl(vector<FieldT> &pol, const vector<FieldT> &a) {
    vec_ZZ_p roots;
    roots.SetLength((long)(a.size()));

    mpz_t rop;
    mpz_init(rop);
    ZZ mid;

    for (size_t i = 0; i < a.size(); i++) {
        libaad::conv_single_fr_zp(a[i], roots[static_cast<long>(i)], rop, mid);
    }
    mpz_clear(rop);

    ZZ_pX polyA (INIT_MONO, 0);
    BuildFromRoots(polyA, roots);

    libaad::convNtlToLibff(polyA, pol);
}
