#include <aad/Configuration.h>
#include <aad/Library.h>
#include <aad/PolyCommit.h>

#include <xassert/XAssert.h>

#include <libff/algebra/scalar_multiplication/multiexp.hpp>

#include <thread>

using namespace libaad;

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);

    // libff naming weirdness that I'm trying to understand:
    // zero() is the group's identity (since ECs use additive notation)
    // one() is the group's generator

    std::clog << "G1 zero: " << G1::zero() << std::endl;
    std::clog << "G1 one: "  << G1::one() << std::endl;
    testAssertNotEqual(G1::one(), G1::zero());

    std::clog << "G2 zero: " << G2::zero() << std::endl;
    std::clog << "G2 one: "  << G2::one() << std::endl;
    testAssertNotEqual(G2::one(), G2::zero());

    // a, b <-$- random
    Fr a = Fr::random_element(), b = Fr::random_element();
    // compute (ab)
    Fr ab = a * b;
    // g1^a
    G1 g1a = a * G1::one();
    // g2^a
    G2 g2b = b * G2::one();
    // gt^(ab)

    // test some simple arithmetic
    testAssertEqual(b*g1a, (ab)*G1::one());
    testAssertEqual(a*g2b, (ab)*G2::one());

    // Get generator in GT
    GT gt = ReducedPairing(G1::one(), G2::one());

    // test pairing
    GT gt_ab = gt^(ab);

    testAssertEqual(
        gt_ab, 
        ReducedPairing(g1a, g2b));

    testAssertEqual(
        gt_ab,
        ReducedPairing(g1a, G2::one())^b);

    testAssertEqual(
        gt_ab,
        ReducedPairing(G1::one(), g2b)^a);

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    // test batch multiple exponentiation
    std::vector<G1> bases1;
    std::vector<G2> bases2;
    std::vector<Fr> exp;
    for(size_t  i = 0; i < 100; i++) {
        bases1.push_back(Fr::random_element() * G1::one());
        bases2.push_back(Fr::random_element() * G2::one());
        exp.push_back(Fr::random_element());
    }

    G1 r1g1 = libff::multi_exp<G1, Fr, libff::multi_exp_method_naive_plain>(
        bases1.cbegin(), bases1.cend(),
        exp.cbegin(), exp.cend(),
        std::thread::hardware_concurrency());
    G2 r1g2 = libff::multi_exp<G2, Fr, libff::multi_exp_method_naive_plain>(
        bases2.cbegin(), bases2.cend(),
        exp.cbegin(), exp.cend(),
        std::thread::hardware_concurrency());

    //method = libff::multi_exp_method_naive;
    G1 r2g1 = libff::multi_exp<G1, Fr, libff::multi_exp_method_naive>(
        bases1.cbegin(), bases1.cend(),
        exp.cbegin(), exp.cend(),
        std::thread::hardware_concurrency());
    G2 r2g2 = libff::multi_exp<G2, Fr, libff::multi_exp_method_naive>(
        bases2.cbegin(), bases2.cend(),
        exp.cbegin(), exp.cend(),
        std::thread::hardware_concurrency());

    //method = libff::multi_exp_method_BDLO12; 
    G1 r3g1 = libff::multi_exp<G1, Fr, libff::multi_exp_method_BDLO12>(
        bases1.cbegin(), bases1.cend(),
        exp.cbegin(), exp.cend(),
        std::thread::hardware_concurrency());
    G2 r3g2 = libff::multi_exp<G2, Fr, libff::multi_exp_method_BDLO12>(
        bases2.cbegin(), bases2.cend(),
        exp.cbegin(), exp.cend(),
        std::thread::hardware_concurrency());

    //method = libff::multi_exp_method_bos_coster;
    G1 r4g1 = libff::multi_exp<G1, Fr, libff::multi_exp_method_bos_coster>(
        bases1.cbegin(), bases1.cend(),
        exp.cbegin(), exp.cend(),
        std::thread::hardware_concurrency());
    G2 r4g2 = libff::multi_exp<G2, Fr, libff::multi_exp_method_bos_coster>(
        bases2.cbegin(), bases2.cend(),
        exp.cbegin(), exp.cend(),
        std::thread::hardware_concurrency());

    G1 r5g1 = multiExp(bases1, exp);
    G2 r5g2 = multiExp(bases2, exp);


    testAssertEqual(r1g1, r2g1);
    testAssertEqual(r2g1, r3g1);
    testAssertEqual(r3g1, r4g1);
    testAssertEqual(r4g1, r5g1);

    testAssertEqual(r1g2, r2g2);
    testAssertEqual(r2g2, r3g2);
    testAssertEqual(r3g2, r4g2);
    testAssertEqual(r4g2, r5g2);
    return 0;
}
