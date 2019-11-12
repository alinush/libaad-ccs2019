#pragma once

#include <aad/EllipticCurves.h>

using namespace std;
using libaad::ReducedPairing;
using libaad::Fr;
using libaad::G1;
using libaad::G2;

namespace NTL {
    class ZZ;
}

namespace libaad {

/*
 * Initializes the library, including its randomness.
 *
 * @param secParam  the security parameter: insecure with probability 2^{-secParam}
 */
void initialize(unsigned char * randSeed, int size = 0);

using NTL::ZZ;

int getSecParam();

} // end of namespace libaad
