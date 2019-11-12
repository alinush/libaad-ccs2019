#include <aad/Configuration.h>

#include <aad/Library.h>
#include <aad/Hashing.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>
#include <xutils/Log.h>

using namespace libaad;
using namespace libaad;

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);
    
    size_t numIters = 1024*16;
    AveragingTimer t1("Hashing a key-value pair");
    std::string keyBase = "your average user name";
    std::string valueBase = "yourAveragePublicKey1234567890123456789012345678901234567890123456789012345678901234567890";
    for(size_t i = 0; i < numIters; i++) {
        std::string key = keyBase + std::to_string(i);
        std::string value = valueBase + std::to_string(i);
        t1.startLap();
        hashKeyValuePair(key, value, static_cast<int>(i));
        t1.endLap();
    }

    logperf << t1 << endl;

    AveragingTimer t2("Hash to field");
    for(size_t i = 0; i < numIters; i++) {
        BitString bs(static_cast<int>(i), 512);
        t2.startLap();
        hashToField(bs);
        t2.endLap();
    }
    logperf << t2 << endl;

    std::cout << "Bench '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}
