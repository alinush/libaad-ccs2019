#include <aad/Configuration.h>

#include <aad/Library.h>
#include <aad/AccumulatedTree.h>
#include <aad/BitString.h>

#include <xassert/XAssert.h>

#include <algorithm>

using namespace libaad;
using libaad::AccumulatedTree;
using libaad::BitString;
using libaad::Node;
using libaad::DataNode;
using std::endl;

void assertEqualFrontiers(const std::set<BitString>& expected, const std::vector<BitString>& actual)
{
    std::set<BitString> actualSet;
    actualSet.insert(actual.begin(), actual.end());
    testAssertTrue(expected == actualSet);
}

void testFrontierSize();

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);

    testFrontierSize();

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

void testFrontierSize() {
    AccumulatedTree tree(4);
    tree.appendPath("0000");
    tree.appendPath("0001");
    tree.appendPath("1000");
    tree.appendPath("1100");
    
    std::set<BitString> expectedFrontier = { "01", "001", "101", "111", "1001", "1101" };
    std::vector<BitString> frontier;
    tree.getFullFrontier(frontier);
    assertEqualFrontiers(expectedFrontier, frontier);
}
