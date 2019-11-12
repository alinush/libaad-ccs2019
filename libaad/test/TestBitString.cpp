#include <aad/Configuration.h>

#include <aad/BitString.h>
#include <aad/Hashing.h>  
#include <aad/Library.h>

#include <xassert/XAssert.h>
#include <xutils/Log.h>

using namespace libaad;
using namespace libaad;
using std::endl;

void assertBitStringsMatch(const BitString& a, const BitString& b, size_t begin, size_t end) {
    for(size_t i = begin; i < end; i++) {
        testAssertEqual(a[i], b[i]);
    }
}

void assertBitStringsDontMatch(const BitString& a, const BitString& b, size_t begin, size_t end) {
    for(size_t i = begin; i < end; i++) {
        if(a[i] != b[i])
            return;
    }
    testAssertFail("Bit strings matched but were not supposed to");
}

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);

    loginfo << "Testing equality..." << endl;
    BitString a("010");
    BitString b("010");
    testAssertEqual(a, b);
    b = "0";
    testAssertNotEqual(a, b);
    // Test some suffixes/prefixes
    //a << 0 << 1 << 0;
    //testAssertEqual(a.suffix(3), a);
    //testAssertEqual(a.prefix(3), a);
    //BitString512 zeroOne;
    //zeroOne << 0 << 1;
    //testAssertEqual(a.prefix(2), zeroOne);
    //BitString oneZero;
    //oneZero << 1 << 0;
    //testAssertEqual(a.suffix(2), oneZero);
    //testAssertNotEqual(zeroOne, oneZero);

    // Test copies
    loginfo << "Testing copies..." << endl;
    BitString copy(a);
    testAssertEqual(copy, a);

    loginfo << "Testing manual initialization..." << endl;
    testAssertEqual(BitString(6), BitString("110"));
    testAssertEqual(BitString(6, 4), BitString("0110"));

    loginfo << "Testing low number of bits (should throw and catch)..." << endl;
    try {
        BitString bad(6, 2);
        assertFail("Should have thrown exception: cannot represent 6 with 2 bits");
    } catch(...) {
    }

    // Test hashing
    loginfo << "Testing hashing to bitstring..." << endl;
    BitString hash1b = hashKeyValuePair("k1b", "v1", 1);
    BitString hash1 = hashKeyValuePair("k1", "v1", 1);
    BitString hash2 = hashKeyValuePair("k1", "v1", 2);
    BitString hash3 = hashKeyValuePair("k1", "v2", 3);
    BitString hash4 = hashKeyValuePair("k2", "v3", 4);

    assertBitStringsMatch(hash1, hash2, 0, 256);
    assertBitStringsMatch(hash2, hash3, 0, 256);
    assertBitStringsDontMatch(hash1, hash4, 0, 256);
    assertBitStringsMatch(hash1, hash1b, 256, 512);
    assertBitStringsDontMatch(hash1, hash2, 256, 512);

    // Test siblings and prefixes
    loginfo << "Testing siblings..." << endl;
    BitString bs;  // 01101
    bs << 0 << 1 << 1 << 0;
    BitString sib(bs); // 01100
    bs << 1;
    sib << 0;

    testAssertEqual(bs.sibling(), sib);
    testAssertEqual(sib.sibling(), bs);

    try {
        loginfo << "Testing empty bitstring has no sibling (should throw and catch)..." << endl;
        BitString::empty().sibling();
    } catch(std::exception& e) {
        loginfo << "Empty bit string has no sibling!" << std::endl;
    }

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}
