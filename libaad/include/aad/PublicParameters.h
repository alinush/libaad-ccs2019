#pragma once

#include <vector>
#include <fstream>
#include <utility>

#include <aad/EllipticCurves.h>

#include <libff/common/serialization.hpp>
#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>

#include <xassert/XAssert.h>
#include <xutils/Log.h>

using namespace std;
using libaad::ReducedPairing;
using libaad::G1;
using libaad::G2;
using libaad::Fr;

namespace libaad {

class PublicParameters {
public:
    size_t q;
    std::vector<G1> g1si, g1tausi; // g1^{s^i} and g1^{\tau s^i}
    std::vector<G2> g2si;          // g2^{s^i}
    //std::vector<G2> g2tausi;       // g2^{\tau s^i}
    Fr s, tau;
    G2 g2tau;

protected:
    PublicParameters(size_t q)
        : q(q)
    {
        resize(q);
    }

public:
    /**
     * Reads s, tau and q from trapFile.
     * Then reads the q-PKE parameters from trapFile + "-0", trapFile + "-1", ...
     * and so on, until q parameters are read.
     */
    PublicParameters(const std::string& trapFile, int maxQ = -1, bool progress = true, bool verify = false);

public:
    static std::tuple<Fr, Fr> generateTrapdoors(size_t q, const std::string& outFile);

    // NOTE: used to patch some old pp files with g2^{tau} to avoid regenerating them
    static void regenerateTrapdoors(std::string& outFile);

    static void generate(size_t startIncl, size_t endExcl, const Fr& s, const Fr& tau, const std::string& outFile, bool progress);
    
    void resize(size_t q) {
        g1si.resize(q+1); // g^{s^i} with i from 0 to q, including q
        g1tausi.resize(q+1);
        g2si.resize(q+1);
        //g2tausi.resize(q+1);
    }

    G1 getG1toS() const {
        return g1si[1];
    }

    G1 getG1toTau() const {
        return g1tausi[0];
    }

    G2 getG2toS() const {
        return g2si[1];
    }
    
    G2 getG2toTau() const {
        return g2tau;
    }

    bool operator!=(const PublicParameters& pp) {
        return ! operator==(pp);
    }

    bool operator==(const PublicParameters& pp) {
        if(q != pp.q)
            return false;
        if(s != pp.s)
            return false;
        if(tau != pp.tau)
            return false;
    
        size_t max = static_cast<size_t>(q);
        for(size_t i = 0; i <= max; i++) {
            if(g1si[i] != pp.g1si[i])
                return false;
            if(g1tausi[i] != pp.g1tausi[i])
                return false;
            
            if(g2si[i] != pp.g2si[i])
                return false;
            //if(g2tausi[i] != pp.g2tausi[i])
            //    return false;
        }
    
        return true;
    }
};

}
