#pragma once

#include <iostream>
#include <ctime>
#include <vector>
#include <sstream>

#include <NTL/ZZ.h>

#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pX.h>
#include <NTL/vec_ZZ_p.h>
#include <NTL/vector.h>
#include <NTL/BasicThreadPool.h>

#include <aad/PolyCommit.h>

using NTL::ZZ_pX;
using NTL::ZZ;
using NTL::ZZ_p;

namespace boost {

/**
 * Custom hash function for ZZ's. Needed for unordered_set<ZZ> hash tables used in SparseTrie.
 * Boost auto-magically detects this.
 */
std::size_t hash_value(const ZZ& z);

} // end of boost namespace

namespace libaad {

template<typename FieldT>
void conv_single_fr_zp(const FieldT& a, ZZ_p& b, mpz_t& rop, ZZ& mid) {
    (a.as_bigint()).to_mpz(rop);
    char *s = mpz_get_str(NULL, 10, rop);

    mid = NTL::conv<ZZ>(s);
    b = NTL::conv<ZZ_p>(mid);

    void (*freefunc)(void *, size_t);
    mp_get_memory_functions(NULL, NULL, &freefunc);
    freefunc(s, strlen(s) + 1);
}

template<typename FieldT>
void conv_fr_zp(const vector<FieldT> &a, ZZ_pX &pa)
{
    void (*freefunc)(void *, size_t);
    mp_get_memory_functions(NULL, NULL, &freefunc);
    pa = ZZ_pX(NTL::INIT_MONO, (long)(a.size() - 1));

    mpz_t rop;
    mpz_init(rop);
    ZZ mid;
    
    for (size_t i = 0; i < a.size(); i++) {
        conv_single_fr_zp(a[i], pa[static_cast<long>(i)], rop, mid);
    }

    mpz_clear(rop);
}

/**
 * Converts an NTL ZZ_pX polynomial to libff polynomial.
 * NOTE: Using faster conversion code from libpolycrypto here.
 */
void convNtlToLibff(const ZZ_pX& pn, vector<Fr>& pf);
void convNtlToLibff(const ZZ_p& zzp, Fr& ff);

} // end of libaad namespace
