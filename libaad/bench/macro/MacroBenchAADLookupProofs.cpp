#include <aad/Configuration.h>

#include <cstdlib>
#include <deque>
#include <random>
#include <algorithm>

#include <boost/unordered_map.hpp>

#include <aad/AADS.h>
#include <aad/BitString.h>
#include <aad/Hashing.h>
#include <aad/Library.h>
#include <aad/Utils.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>
#include <xutils/Utils.h>

using namespace libaad;
using namespace libaad;
using std::endl;

/**
 * Output format:
 * dictSize, totalMembBytes, forestMembBytes, frontierMembBytes, totalNonMembBytes,
 */

std::mt19937 *urng;

void benchMembProofSize(int dictSize, int numSamples, const std::vector<int>& numValues, const std::string& fileName, bool progress = false);

int main(int argc, char *argv[])
{
    int numSamples = 10, dictSize = 1023;
    std::string seedStr = "42";
    std::vector<int> numValues = {0, 1, 2, 4, 8, 16, 32};

    initialize(nullptr, 0);

    if(argc > 1) {
        if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            std::cout << "Usage: " << argv[0] << " [dictSize] [randSeed]" << endl;
            return 0;
        }
        auto oldDictSize = dictSize;
        dictSize = std::stoi(argv[1]);
        loginfo << "Chaging dictionary size from " << oldDictSize << " to " << dictSize << endl;
    }
    if(argc > 2) {
        seedStr.assign(argv[2]);
    }
    unsigned int seed = static_cast<unsigned int>(std::stoi(seedStr.c_str()));
    
    loginfo << "Seeding srand() with " << seedStr << "..." << endl;
    srand(seed);
    std::seed_seq seedseq(seedStr.begin(), seedStr.end());
    urng = new std::mt19937(seedseq);
    
    loginfo << "Benchmarking complete membership proof sizes for AADs of size " << dictSize << endl;
    loginfo << "Sampling a different key " << numSamples << " times" << endl;


    std::string fileName = "aad-memb-proof-" + std::to_string(dictSize) + ".csv";
    benchMembProofSize(dictSize, numSamples, numValues, fileName, true);

    printMemUsage("Memory usage before exiting benchmark");

    std::cout << endl << "Bench '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

std::string getKey(int numVals, int sampleNo) {
    return "key/vals=" + std::to_string(numVals) + "/sample=" + std::to_string(sampleNo);
}

void initKeys(std::vector<std::string>& keys, int numSamples, const std::vector<int>& numValues) {
    for(auto nv : numValues) {
        for(int v = 0; v < nv; v++) {
            for(int i = 0; i < numSamples; i++) {
                keys.push_back(getKey(nv, i));
            }
        }
    }
}

void benchMembProofSize(int dictSize, int numSamples, const std::vector<int>& numValues, const std::string& fileName, bool progress) {
    AAD<std::string, std::string> aad;

    // pick random leaves for the key-value pairs
    int numLeaves = std::accumulate(numValues.cbegin(), numValues.cend(), 0) * numSamples;

    if(numLeaves > dictSize) {
        throw std::runtime_error("You are testing too many #'s of values. The necessary key-value pairs don't all fit in the AAD.");
    }

    std::set<int> randomLeaves;
    loginfo << "Picking " << numLeaves << " random leaves in forest of size " << dictSize << endl;
    Utils::randomSubset(randomLeaves, dictSize, numLeaves);

    ofstream fout(fileName);
    fout << "dictSize,numValues,forestBytes,frontierBytes,totalBytes,verifyUsec," << endl;

    // generate random sequence of keys, with repeats, since a key will be inserted multiple times with a different value
    std::vector<std::string> keys;
    initKeys(keys, numSamples, numValues);
    // WARNING: We shuffle the keys using std::shuffle because std::random_shuffle is deprecated in C++14
    std::shuffle(keys.begin(), keys.end(), *urng);

    auto keyIt = keys.begin();
    boost::unordered_map<std::string, std::list<std::string>> keyToValues;

    // returns 2^i such that 2^i <= dictSize
    int batchSize = Utils::greatestPowerOfTwoBelow(dictSize);
    aad.setBatchSize(batchSize);
    logdbg << "Setting batch size to " << batchSize << " (" << dictSize << " elements left)" << endl;
    int nextSizeTarget = batchSize;
    
    int prevPct = -1;
    std::string key, value;
    ManualTimer appendTime;
    for(int i = 0; i < dictSize; i++) {
        if(i == nextSizeTarget) {
            int left = dictSize - i;
            batchSize = Utils::greatestPowerOfTwoBelow(left);
            nextSizeTarget += batchSize;
            aad.setBatchSize(batchSize);
            logdbg << "Setting batch size to " << batchSize << " (" << left << " elements left)" << endl;
        }

        if(randomLeaves.count(i) > 0) {
            if(keyIt == keys.end())
                throw std::logic_error("We were supposed to still have keys to insert. Make sure the 'keys' and 'randomLeaves' array are correctly generated!");
            key = *keyIt;
            keyIt++;

            value = "val/leafNo=" + std::to_string(i) + "/" + key;
            //addValueToKey(keyToValues, key, value);
            keyToValues[key].push_back(value);
        } else {
            // Append (random) key
            key = "ignoredKey/leafNo=" + std::to_string(i);
            value = "ignoredValue/leafNo=" + std::to_string(i);
        }

        aad.append(key, value); 

        //loginfo << "Key " << i << ": " << aad.getKeyByLeafNo(i) << endl;
        
        if(progress) {
            int pct = static_cast<int>(static_cast<double>(i)/static_cast<double>(dictSize) * 100.0);
            if(pct > prevPct) {
                loginfo << pct << "% ... (i = " << i << " out of " << dictSize << ")" << endl;
                prevPct = pct;

                printMemUsage();
                loginfo << endl;
            }
        }
    }
    assertEqual(dictSize, aad.getSize());

    loginfo << "Reached size " << dictSize << " in " << appendTime.stop().count() / 1000000 << " seconds, computing membership proofs..." << endl;
    loginfo << endl;
    
    std::list<std::string> values;
    for(auto nv : numValues) {
        Digest digest = aad.getDigest();
        AveragingTimer timeMemb;

        for(int j = 0; j < numSamples; j++) {
            if(nv == 0) {
                key = "noSuchKey/sample=" + std::to_string(j);
                loginfo << "Non-membership proof for '" << key << "'" << endl;

                values = std::list<std::string>();
            } else {
                key = getKey(nv, j);
                loginfo << "Membership proof for '" << key << "' with " << nv << " values (sample = " << j << ")" << endl;

                if(keyToValues.count(key) == 0)
                    throw std::logic_error("Expected key to be in 'keyToValues' map");
                values = keyToValues[key];
                assertEqual(values.size(), static_cast<size_t>(nv));
            }

            // do complete membership proof and get its size
            auto mp = aad.completeMembershipProof(key);
            fout << dictSize << ", " << std::flush;
            fout << nv << ", " << std::flush;
            fout << mp->getForestProofSize() << ", " << std::flush;
            fout << mp->getFrontierProofSize() << ", " << std::flush;
            fout << mp->getProofSize() << ", " << std::flush;
            
            if(nv == 0) {
                testAssertTrue(mp->isNonMembershipProof());
                testAssertEqual(mp->getForestProofSize(), 0);
            }

            // time memb proof verification
            timeMemb.startLap();
            testAssertTrue(mp->verify(key, values, digest));
            auto membVerTime = timeMemb.endLap();

            fout << membVerTime << ", " << std::flush;
            fout << std::endl;
        }
    
        logperf << "Membership proof verification time " << timeMemb.averageLapTime() << " microseconds" << endl;
    }
}
