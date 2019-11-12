#include <aad/Configuration.h>

#include <aad/PolyCommit.h>
#include <aad/PublicParameters.h>

#include <xutils/Log.h>
#include <xutils/Timer.h>

#include <libff/algebra/scalar_multiplication/multiexp.hpp>

#include <thread>

using namespace std;

namespace libaad {

template<class Group>
Group multiExp(
    typename std::vector<Group>::const_iterator base_begin, 
    typename std::vector<Group>::const_iterator base_end, 
    typename std::vector<Fr>::const_iterator exp_begin,
    typename std::vector<Fr>::const_iterator exp_end
    )
{
    long sz = base_end - base_begin;
    long expsz = exp_end - exp_begin;
    assertEqual(sz, expsz);
    size_t numCores = getNumCores();

    if(sz > 4) {
        if(sz > 16384) {
            return libff::multi_exp<Group, Fr, libff::multi_exp_method_BDLO12>(base_begin, base_end,
                exp_begin, exp_end, numCores);
        } else {
            return libff::multi_exp<Group, Fr, libff::multi_exp_method_bos_coster>(base_begin, base_end,
                exp_begin, exp_end, numCores);
        }
    } else {
        return libff::multi_exp<Group, Fr, libff::multi_exp_method_naive>(base_begin, base_end,
            exp_begin, exp_end, 1);
    }
}

template G1 multiExp<G1>(
    std::vector<G1>::const_iterator base_begin,
    std::vector<G1>::const_iterator base_end, 
    std::vector<Fr>::const_iterator exp_begin,
    std::vector<Fr>::const_iterator exp_end
);

template G2 multiExp<G2>(
    std::vector<G2>::const_iterator base_begin,
    std::vector<G2>::const_iterator base_end, 
    std::vector<Fr>::const_iterator exp_begin,
    std::vector<Fr>::const_iterator exp_end
);

template<class Group>
Group multiExp(
    const std::vector<Group>& bases, 
    const std::vector<Fr>& exps) 
{
    assertEqual(bases.size(), exps.size());
    return multiExp<Group>(bases.cbegin(), bases.cend(),
        exps.cbegin(), exps.cend());
}

template G1 multiExp(
    const std::vector<G1>& bases, 
    const std::vector<Fr>& exps
);

template G2 multiExp(
    const std::vector<G2>& bases, 
    const std::vector<Fr>& exps
);

size_t getNumCores() {
    static size_t numCores = std::thread::hardware_concurrency();
    if(numCores == 0)
        throw std::runtime_error("Could not get number of cores");
    return numCores;
}

void PolyCommit::checkDegree(const PublicParameters& pp, const vector<Fr>& poly) {
    // e.g., p(x) = (x-1)(x-3)(x-5)(x-2) with degree 4 and 5 coefficients but q = 3 (i.e., g, g^s, g^{s^2} and g^{s^3})
    assertStrictlyPositive(poly.size());
    size_t degree = poly.size() - 1;
    if(degree > pp.q) {
        logerror << "Poly has " << poly.size() << " coefficients but we only have q = " << pp.q << " PKE parameters" << endl;
        throw std::runtime_error("Do not have enough q-PKE parameters");
    }
}

G1 PolyCommit::commitG1(const PublicParameters& pp, const vector<Fr>& poly, bool isExtractable)
{
    checkDegree(pp, poly);
    G1 g1comm;

    //{
    //    logperf << (isExtractable ? "Extractable" : "Non-extract.");
    //    ScopedTimer<std::chrono::milliseconds> t1(std::cout, " commitG1 took ");
        if(isExtractable) {
            auto g1tausi_start = pp.g1tausi.cbegin();
            auto g1tausi_end = g1tausi_start + static_cast<long>(poly.size());
            g1comm = multiExp<G1>(g1tausi_start, g1tausi_end, 
                poly.cbegin(), poly.cend());
        } else {
            auto g1si_start = pp.g1si.cbegin();
            auto g1si_end = g1si_start + static_cast<long>(poly.size());
            g1comm = multiExp<G1>(g1si_start, g1si_end, 
                poly.cbegin(), poly.cend());
        }
    //}
    //std::cout << std::flush;
    return g1comm;
}

G2 PolyCommit::commitG2(const PublicParameters& pp, const vector<Fr>& poly, bool isExtractable)
{
    checkDegree(pp, poly);
    G2 g2comm;

    //{
    //    logperf << "Non-extract.";
    //    ScopedTimer<std::chrono::milliseconds> t1(std::cout, " commitG2 took ");
        auto g2si_start = pp.g2si.cbegin();
        auto g2si_end = g2si_start + static_cast<long>(poly.size());
        g2comm = multiExp<G2>(g2si_start, g2si_end, poly.cbegin(), poly.cend());
    //}
    //std::cout << std::flush;

    if(isExtractable) {
        throw std::runtime_error("We do not support extractable commits in G2");
        //g2ExtrComm = poly[i] * pp.g2tausi[i] + g2ExtrComm;
    }
    
    return g2comm;
}

} // end of namespace libaad
