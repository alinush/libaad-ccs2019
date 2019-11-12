#include <aad/Configuration.h>

#include <aad/Library.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>
#include <xutils/Log.h>

using namespace libaad;

template<class G>
void benchGroupExp(int iters) {
    G a, b, c;
    Fr e;
    AveragingTimer t("Exponentiation with random bases");
    for(int i = 0; i < iters; i++) {
        a = G::random_element(); 
        e = Fr::random_element();
      
        t.startLap();
        c = e*a;
        t.endLap();
    }
    logperf << t << endl;
    
    AveragingTimer t1("Exponentiation with generator as base");
    for(int i = 0; i < iters; i++) {
        e = Fr::random_element();
      
        t1.startLap();
        c = e*G::one();
        t1.endLap();
    }
    logperf << t1 << endl;
}

void benchPairing(int iters) {
    
    // Get generator in GT
    GT gt = ReducedPairing(G1::one(), G2::one());
    GT gt_ab_pair;

    AveragingTimer t1("Pairing on random elements");
    // a, b <-$- random
    Fr a, b;
    for(int i = 0; i < iters; i++) {
        a = Fr::random_element();
        b = Fr::random_element();
        Fr ab = a * b;
        G1 g1a = a * G1::one();
        G2 g2b = b * G2::one();
     
        // test some simple arithmetic
        testAssertEqual(b*g1a, (ab)*G1::one());
        testAssertEqual(a*g2b, (ab)*G2::one());
        
        t1.startLap(); 
        gt_ab_pair = ReducedPairing(g1a, g2b);
        t1.endLap();

        testAssertEqual(gt^(ab), gt_ab_pair);
    }
    logperf << t1 << endl;

    AveragingTimer t2("Pairing on g1, g2");
    for(int i = 0; i < iters; i++) {
        t2.startLap(); 
        ReducedPairing(G1::one(), G2::one());
        t2.endLap();
    }
    logperf << t2 << endl;
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);

    // libff naming weirdness that I'm trying to understand:
    // zero() is the group's identity (since ECs use additive notation)
    // one() is the group's generator
    logperf << endl;
    logperf << "Benchmarking G1... " << endl;
    benchGroupExp<G1>(1024);

    logperf << endl;
    logperf << "Benchmarking G2... " << endl;
    benchGroupExp<G2>(1024);
    
    logperf << endl;
    logperf << "Benchmarking pairing... " << endl;
    benchPairing(1024);

    std::cout << "Bench '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}
