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

void testLowerFrontier();
void testFrontier();
void testAccumulatedTree();

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);

    testFrontier();
    testLowerFrontier();
    testAccumulatedTree();

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

void testLowerFrontier() {
    std::vector<BitString> vec;
    std::set<BitString> expected, lowFrontier;
    AccumulatedTree tree(4);
    tree.appendPath("0000");
    tree.appendPath("0010");
    tree.appendPath("1101");
    tree.appendPath("1111");

    vec.clear();
    tree.getLowerFrontier(vec, "00");
    lowFrontier.clear();
    lowFrontier.insert(vec.begin(), vec.end());
    expected = {"0001", "0011"};
    testAssertTrue(lowFrontier == expected);

    vec.clear();
    tree.getLowerFrontier(vec, "11");
    lowFrontier.clear();
    lowFrontier.insert(vec.begin(), vec.end()); 
    expected = {"1100", "1110"};
    //std::ostream_iterator<BitString> coutit(std::cout, ", ");
    //std::copy(lowFrontier.begin(), lowFrontier.end(), coutit);
    //std::cout << endl;
    testAssertTrue(lowFrontier == expected);

    // Test lower and upper frontiers are equal to the full frontier
    std::set<BitString> fullFrontier;
    tree.getFullFrontier(vec);
    expected.clear(); 
    expected.insert(vec.begin(), vec.end());

    // Get lower frontier using prefixes manually
    tree.getUpperFrontier(vec);
    testAssertTrue(vec.size() > 0);
    fullFrontier.insert(vec.begin(), vec.end());
    vec.clear();
    tree.getLowerFrontier(vec, "00");
    testAssertTrue(vec.size() > 0);
    fullFrontier.insert(vec.begin(), vec.end());
    vec.clear();
    tree.getLowerFrontier(vec, "11");
    testAssertTrue(vec.size() > 0);
    fullFrontier.insert(vec.begin(), vec.end());
    testAssertTrue(expected == fullFrontier);
    
    // Get lower frontier using roots returned from getUpperFrontier
    fullFrontier.clear();
    std::vector<Node*> lowerRoots;
    tree.getUpperFrontier(vec, lowerRoots);
    testAssertTrue(vec.size() > 0);
    fullFrontier.insert(vec.begin(), vec.end());
    for(auto lowRoot : lowerRoots) {
        BitString keyHash = lowRoot->getLabel();

        vec.clear();
        tree.getLowerFrontier(vec, keyHash, lowRoot);
        testAssertTrue(vec.size() > 0);
        fullFrontier.insert(vec.begin(), vec.end());
    }
    testAssertTrue(expected == fullFrontier);
    
    //std::ostream_iterator<BitString> coutit(std::cout, ", ");
    //std::copy(fullFrontier.begin(), fullFrontier.end(), coutit);
    //std::cout << endl;
}

void testFrontier() {
    // Create a tree of max depth 4 (i.e., 4 edges)
    AccumulatedTree tree(4);
    std::set<BitString> frontier, expectedFrontier;
    std::vector<BitString> vec;

    // Now tree has 10 in it
    tree.appendPath("10");
    tree.getFullFrontier(vec);
    frontier.clear();
    frontier.insert(vec.begin(), vec.end());
    expectedFrontier = { "0", "11", "100", "101" };
    testAssertTrue(frontier == expectedFrontier);

    bool found;
    BitString prefix;
    std::tie(found, std::ignore, std::ignore) = tree.containsKey("");
    testAssertTrue(found);
    std::tie(found, std::ignore, std::ignore) = tree.containsKey("1");
    testAssertTrue(found);
    std::tie(found, std::ignore, std::ignore) = tree.containsKey("10");
    testAssertTrue(found);
    std::tie(found, std::ignore, prefix) = tree.containsKey("0");
    testAssertFalse(found);
    testAssertEqual(prefix, "0");
    std::tie(found, std::ignore, prefix) = tree.containsKey("00");
    testAssertFalse(found);
    testAssertEqual(prefix, "0");
    std::tie(found, std::ignore, prefix) = tree.containsKey("11");
    testAssertFalse(found);
    testAssertEqual(prefix, "11");
    std::tie(found, std::ignore, prefix) = tree.containsKey("100");
    testAssertFalse(found);
    testAssertEqual(prefix, "100");

    // Now tree has 1001 in it
    tree.appendPath("1001");
    tree.getFullFrontier(vec);
    frontier.clear();
    frontier.insert(vec.begin(), vec.end());
    expectedFrontier = { "0", "11", "101", "1000" };
    testAssertTrue(frontier == expectedFrontier);
    
    std::tie(found, std::ignore, std::ignore) = tree.containsKey("100");
    testAssertTrue(found);
    std::tie(found, std::ignore, prefix) = tree.containsKey("1001111"); // goes past max depth of 4, but we allow it
    testAssertFalse(found);
    testAssertEqual(prefix, "10011");
    std::tie(found, std::ignore, prefix) = tree.containsKey("10100"); // goes past max depth of 4, but we allow it
    testAssertFalse(found);
    testAssertEqual(prefix, "101");
    std::tie(found, std::ignore, prefix) = tree.containsKey("1000");
    testAssertFalse(found);
    testAssertEqual(prefix, "1000");

    // now tree has 1000 and 1001 in it
    tree.appendPath("1000");
    tree.getFullFrontier(vec);
    frontier.clear();
    frontier.insert(vec.begin(), vec.end());
    expectedFrontier.erase("1000");
    testAssertTrue(frontier == expectedFrontier);

    std::tie(found, std::ignore, prefix) = tree.containsKey("101");
    testAssertFalse(found);
    testAssertEqual(prefix, "101");
    std::tie(found, std::ignore, std::ignore) = tree.containsKey("1000");
    testAssertTrue(found);
    std::tie(found, std::ignore, std::ignore) = tree.containsKey("1001");
    testAssertTrue(found);
}

void testAccumulatedTree() {
    std::unique_ptr<AccumulatedTree> tree1(new AccumulatedTree(4, "1000"));
    std::vector<BitString> prefixes = { BitString::empty(), "1", "10", "100", "1000" };
    assertTrue(prefixes == tree1->getPrefixes()); 

    std::unique_ptr<AccumulatedTree> tree2(new AccumulatedTree(4, "0001"));
    prefixes = { BitString::empty(), "0", "00", "000", "0001" };
    assertTrue(prefixes == tree2->getPrefixes()); 

    AccumulatedTree merged(std::move(tree1), std::move(tree2));
    prefixes = { BitString::empty(), "0", "1", "00", "10", "000", "100", "0001", "1000" };
    //std::cout << "Expected prefixes: ";
    //std::copy(prefixes.begin(), prefixes.end(), std::ostream_iterator<BitString>(std::cout, ", "));
    //std::cout << endl;
    auto mergedPrefixes = merged.getPrefixes(); 
    std::sort(mergedPrefixes.begin(), mergedPrefixes.end());
    //std::cout << "Merged prefixes:   ";
    //std::copy(mergedPrefixes.begin(), mergedPrefixes.end(), std::ostream_iterator<BitString>(std::cout, ", "));
    //std::cout << endl;
    assertTrue(prefixes == mergedPrefixes);

    std::set<BitString> expected = {"1000", "0001"};
    for(auto label : expected) {
        logdbg << "Checking if node " << label << " is in the merged AT ..." << endl;
        testAssertTrue(std::get<0>(merged.containsKey(label)));
    }
    
    std::vector<BitString> fullFrontier;
    merged.getFullFrontier(fullFrontier);
    expected = {"11", "101", "1001", "01", "001", "0000"};
    for(auto label : fullFrontier) {
        testAssertTrue(expected.count(label) > 0);
        expected.erase(label);
    }

    testAssertTrue(expected.empty());
}
