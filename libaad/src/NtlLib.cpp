#include <aad/Configuration.h>

#include <aad/NtlLib.h>
#include <aad/Library.h>
#include <aad/BitString.h>
#include <aad/PicoSha2.h>

#include <xutils/AutoBuf.h>
#include <xutils/Timer.h>

#include <boost/functional/hash.hpp>

namespace boost {

std::size_t hash_value(const ZZ& z)
{
    AutoBuf<unsigned char> buf(NTL::NumBytes(z));
    NTL::BytesFromZZ(buf, z, buf.size());
	return boost::hash_range<unsigned char*>(buf, buf + buf.size());
}

} // end of namespace boost

namespace libaad {

void convNtlToLibff(const NTL::vec_ZZ_p& pn, vector<Fr>& pf) {
    // allocate enough space for libff polynomial
    pf.resize(static_cast<size_t>(pn.length()));

    mpz_t interm;
    mpz_init(interm);

    long numBytes = NumBytes(ZZ_p::modulus());
    AutoBuf<unsigned char> buf(numBytes);
    buf.zeroize();

    for (size_t i = 0; i < pf.size(); i++) {
        const ZZ_p& coeff = pn[static_cast<long>(i)];
        const ZZ& coeffRep = NTL::rep(coeff);

        // NOTE: Puts the least significant byte in buf[0]
        NTL::BytesFromZZ(buf, coeffRep, numBytes);

        // convert byte array to mpz_t
        mpz_import(interm,
            /*count =*/static_cast<size_t>(numBytes),
            /*order =*/-1,
            /*size  =*/1,
            /*endian=*/0,
            /*nails =*/0,
            buf);

        pf[i] = libff::bigint<Fr::num_limbs>(interm);
    }

    mpz_clear(interm);
}

void convNtlToLibff(const ZZ_pX& pn, vector<Fr>& pf) {
    convNtlToLibff(pn.rep, pf);
}

void convNtlToLibff(const ZZ_p& zzp, Fr& ff) {
    long numBytes = NumBytes(ZZ_p::modulus());
    AutoBuf<unsigned char> buf(numBytes);
    buf.zeroize();

    mpz_t interm;
    mpz_init(interm);

    const ZZ& rep = NTL::rep(zzp);

    // NOTE: Puts the least significant byte in buf[0]
    NTL::BytesFromZZ(buf, rep, numBytes);

    // convert byte array to mpz_t
    mpz_import(interm,
        /*count =*/static_cast<size_t>(numBytes),
        /*order =*/-1,
        /*size  =*/1,
        /*endian=*/0,
        /*nails =*/0,
        buf);

    ff = libff::bigint<Fr::num_limbs>(interm);
    
    mpz_clear(interm);
}

} // end of namespace libaad
