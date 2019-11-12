#include <aad/Configuration.h>

#include <cstdlib>

#include <aad/Library.h>
#include <aad/PublicParameters.h>

#include <xassert/XAssert.h>
#include <xutils/Timer.h>
#include <xutils/Log.h>

using namespace libaad;
using namespace libaad;
using std::endl;

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;
    initialize(nullptr, 0);
    srand(42);

    size_t q = argc < 2 ? 4096 : static_cast<size_t>(std::stoi(argv[1]));
    size_t numChunks = 5;
    if(q < numChunks)
        throw std::runtime_error("q must be bigger than the # of chunks");
    size_t chunkSize = q / numChunks;

    std::string trapFile = "/tmp/libaad-test-trapFile";
    auto tds = PublicParameters::generateTrapdoors(q, trapFile);
    Fr s = std::get<0>(tds);
    Fr tau = std::get<1>(tds);

    for(size_t i = 0; i < numChunks; i++) {
        size_t start = i*chunkSize;
        size_t end = (i < numChunks-1) ? (i+1)*chunkSize : q+1;
        
        logdbg << "Writing chunk #" << i+1 << ": [" << start << ", " << end << ") ..." << endl;
        PublicParameters::generate(start, end, s, tau, trapFile + "-" + std::to_string(i), false); 
    }

    PublicParameters pp(trapFile);

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}
