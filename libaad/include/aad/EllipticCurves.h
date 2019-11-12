#pragma once

#include <utility>

#include <libff/algebra/curves/public_params.hpp>
#include <libff/common/default_types/ec_pp.hpp>

namespace libaad {
    // Type of group G1
    using G1 = typename libff::default_ec_pp::G1_type;
    // Type of group G2
    using G2 = typename libff::default_ec_pp::G2_type;
    // Type of group GT (recall pairing e : G1 x G2 -> GT)
    using GT = typename libff::default_ec_pp::GT_type;
    // Type of the finite field "in the exponent" of the EC group elements
    using Fr = typename libff::default_ec_pp::Fp_type;

    // Pairing function, takes an element in G1, another in G2 and returns the one in GT
    //using libff::default_ec_pp::reduced_pairing;
    //using ECPP::reduced_pairing;
    // Found this solution on SO: https://stackoverflow.com/questions/9864125/c11-how-to-alias-a-function
    // ('using ECPP::reduced_pairing;' doesn't work, even if you expand ECPP)
    template <typename... Args>
    auto ReducedPairing(Args&&... args) -> decltype(libff::default_ec_pp::reduced_pairing(std::forward<Args>(args)...)) {
        return libff::default_ec_pp::reduced_pairing(std::forward<Args>(args)...);
    }
    
    constexpr static int G1ElementSize = 32; // WARNING: Assuming BN128 curve
    constexpr static int G2ElementSize = 64; // WARNING: Assuming BN128 curve
}
