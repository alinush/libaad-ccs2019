#pragma once

#include <vector>
#include <fstream>

#include <aad/EllipticCurves.h>

#include <xassert/XAssert.h>
#include <xutils/Log.h>

using namespace std;
using libaad::ReducedPairing;
using libaad::Fr;
using libaad::G1;
using libaad::G2;

namespace libaad {

class PublicParameters;

size_t getNumCores(); 

template<class Group>
Group multiExp(
    typename std::vector<Group>::const_iterator base_begin, 
    typename std::vector<Group>::const_iterator base_end, 
    typename std::vector<Fr>::const_iterator exp_begin,
    typename std::vector<Fr>::const_iterator exp_end
);

template<class Group>
Group multiExp(
    const std::vector<Group>& bases, 
    const std::vector<Fr>& exps
); 
 
class PolyCommit {
public:
    static void checkDegree(const PublicParameters& pp, const vector<Fr>& poly);

    static G1 commitG1(const PublicParameters& pp, const vector<Fr>& poly, bool isExtractable);
    static G2 commitG2(const PublicParameters& pp, const vector<Fr>& poly, bool isExtractable = false);
};

} // end of namespace libaad
