#include <aad/Configuration.h>
#include <aad/Library.h>

#include <aad/PolyOps.h>
#include <aad/Utils.h>
#include <aad/EllipticCurves.h>

#include <cstdlib>
#include <vector>
#include <cmath>
#include <iostream>

#include <libfqfft/polynomial_arithmetic/basic_operations.hpp>
#include <libff/algebra/scalar_multiplication/multiexp.hpp>
#include <libff/common/double.hpp>

#include <xassert/XAssert.h>
#include <xutils/Log.h>
#include <xutils/Timer.h>

using namespace std;
using namespace libfqfft;
using namespace libaad;

template<class Group>
void benchMultiExp(const std::string& method, const std::vector<Group>& g1, const std::vector<Fr> exp, int numIters, size_t numCores) {
    std::string timerName = method + ", " + std::to_string(exp.size()) + " exps, " + std::to_string(numIters) + " iters, " + std::to_string(numCores) + " cores: ";
    AveragingTimer t(timerName);

    for(int i = 0; i < numIters; i++) {
        t.startLap();
        if(method == "naive_plain")
        {
            // This is the most naive method: inner loop that does the exponentiations one by one
            libff::multi_exp<Group, Fr, libff::multi_exp_method_naive_plain>(g1.cbegin(), g1.cend(), exp.cbegin(), exp.cend(), numCores);
        }
        else if(method == "naive")
        {
            libff::multi_exp<Group, Fr, libff::multi_exp_method_naive>(g1.cbegin(), g1.cend(), exp.cbegin(), exp.cend(), numCores);
        }
        else if(method == "bos_coster") 
        {
            libff::multi_exp<Group, Fr, libff::multi_exp_method_bos_coster>(g1.cbegin(), g1.cend(), exp.cbegin(), exp.cend(), numCores);
        }
        else if(method == "bdlo12")
        {
            libff::multi_exp<Group, Fr, libff::multi_exp_method_BDLO12>(g1.cbegin(), g1.cend(), exp.cbegin(), exp.cend(), numCores);
        }
        else
        {
            throw std::runtime_error("Invalid multi-exponentiation method name");
        }
        t.endLap();
    }

    logperf << t << endl;
    logperf << " * Average time per exponentiation: " << static_cast<size_t>(t.averageLapTime()) / g1.size() << " microsecs" << endl;
}

int main(int argc, char * argv[])
{
    libaad::initialize(nullptr, 0);

    srand(static_cast<unsigned int>(time(NULL)));

    if((argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) || argc < 4) {
        cout << endl;
        cout << "Usage: " << argv[0] << " <method> <num-exps> <num-iters> [<skip-G2>]" << endl;
        cout << endl;
        cout << "<method> can be either 'naive_plain', 'naive', 'bos_coster' or 'bdlo12'" << endl;
        cout << "<skip-G2> can be either 0 or 1 (default 1)" << endl;
        cout << endl;
        return 1;
    }
    
    std::string method = argv[1];
    int n = std::stoi(argv[2]);
    int numIters = std::stoi(argv[3]);
    bool skipG2 = true;
    if(argc > 4)
        skipG2 = std::stoi(argv[4]) == 1;

    vector<Fr> exp;
    vector<G1> g1;
    vector<G2> g2;

    size_t numCores = getNumCores();

    loginfo << "Number of cores: " << numCores << endl;

    g1.reserve(static_cast<size_t>(n));
    g2.reserve(static_cast<size_t>(n));
    exp.reserve(static_cast<size_t>(n));

    loginfo << "Picking " << n << " random elements..." << endl;
    for(int i = 0; i < n; i++) {
        g1.push_back(G1::random_element());
        g2.push_back(G2::random_element());
        exp.push_back(Fr::random_element());
    }
    loginfo << endl;
    
    logperf << "Benchmarking '" << method << "' MultiExp in G1..." << endl;
    benchMultiExp(method, g1, exp, numIters, 1);
    benchMultiExp(method, g1, exp, numIters, numCores);

    if(!skipG2) {
        logperf << endl;
        logperf << "Benchmarking '" << method << "' MultiExp in G2..." << endl;
        benchMultiExp(method, g2, exp, numIters, 1);
        benchMultiExp(method, g2, exp, numIters, numCores);
    }

    return 0;
}
