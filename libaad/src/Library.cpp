#include <aad/Configuration.h>

#include <aad/Library.h>
#include <aad/NtlLib.h>

#include <xutils/Log.h>

#include <libff/common/profiling.hpp>

using namespace std;

namespace libaad {

static int g_secParam = -1;

void initialize(unsigned char * randSeed, int size)
{
    (void)randSeed; // FIXME: initialize entropy source
    (void)size;     // FIXME: initialize entropy source

    // Apparently, libff logs some extra info when computing pairings
    libff::inhibit_profiling_info = true;

    // Initializes the default EC curve, so as to avoid "surprises"
    libff::default_ec_pp::init_public_params();

    // Initializes the NTL finite field to be the same as libff's for BN128
    ZZ p = NTL::conv<ZZ> ("21888242871839275222246405745257275088548364400416034343698204186575808495617");
    ZZ_p::init(p);
}

int getSecParam() { return g_secParam; }

} // end of namespace libaad
