#pragma once

#include <vector>
#include <utility>

#include <aad/AccumulatedTree.h>
#include <aad/Hashing.h>
#include <aad/PolyCommit.h>
#include <aad/PolyInterpolation.h>

#include <xassert/XAssert.h>
#include <xutils/Log.h>
#include <xutils/Timer.h>

namespace libaad {

class CommitUtils {
public:
    template<class Group>
    static Group simulateCommitment(const std::vector<Fr>& exp) {
        std::vector<Group> bases;
        for(size_t i = 0; i < exp.size(); i++) {
            bases.push_back(Group::one());
        }
        return multiExp<Group>(bases, exp);
    }

    static std::tuple<G1, G1> commitAT(AccumulatedTree * at, const PublicParameters* pp, bool extractable) {
        std::vector<Fr> accPoly;
        return commitAT(at, accPoly, pp, extractable);
    }

    static std::tuple<G1, G1> commitAT(AccumulatedTree * at, std::vector<Fr>& accPoly, const PublicParameters* pp, bool extractable) {
        G1 acc, eAcc = G1::one();

        // get roots of AT polynomial
        ManualTimer t;
        std::vector<BitString> prefixes = at->getPrefixes();
        std::vector<Fr> hashes;
        hashToField(prefixes, hashes);
        auto micros = t.stop().count();
        printOpPerf(micros, "hash_AT_prefixes", prefixes.size()); 
        std::vector<BitString>().swap(prefixes);    // clear prefixes

        // interpolate AT polynomial
        assertIsZero(accPoly.size());
        t.restart();
        poly_from_roots_ntl(accPoly, hashes);
        assertEqual(accPoly.size(), hashes.size() + 1);
        micros = t.stop().count();
        printOpPerf(micros, "interpolate_AT", accPoly.size());
        std::vector<Fr>().swap(hashes);      // clear hashes

        // commit to polynomial
        t.restart();
        acc = pp != nullptr ? PolyCommit::commitG1(*pp, accPoly, false) : simulateCommitment<G1>(accPoly);
        if(extractable)
            eAcc = pp != nullptr ? PolyCommit::commitG1(*pp, accPoly, true) : simulateCommitment<G1>(accPoly);
        micros = t.stop().count();
        printOpPerf(micros, (extractable ? "commitExtr" : "commitNoEx"), accPoly.size()); 

        return std::make_tuple(acc, eAcc);
    }
};

}
