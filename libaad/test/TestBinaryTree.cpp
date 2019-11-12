#include <aad/Configuration.h>

#include <aad/Library.h>
#include <aad/BinaryTree.h>
#include <aad/BitString.h>

#include <xassert/XAssert.h>
#include <xutils/Log.h>

using namespace libaad;
using namespace libaad;
using std::endl;

void testBinaryForest();

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);

    testBinaryForest();

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}

void testBinaryForest()
{
    class AddFunc {
    public:
        int* operator() (DataNode<int>* left, DataNode<int>* right, bool isLastMerge) {
            (void)isLastMerge;

            int* a = left->getData();
            int* b = right->getData();
            testAssertNotNull(a);
            testAssertNotNull(b);
            return new int(*a + *b);
        }
    };

    AddFunc mergeFunc;
    BinaryForest<int, AddFunc> forest;
    forest.setMergeFunc(&mergeFunc);

    forest.appendLeaf(NodeFactory::makeNode(new int(1)));
    logdbg << "Appending leaf with i = " << 1 << endl;
    auto expectedRoots = forest.getRoots();

    for(int i = 2; i < 100; i++) {
        logdbg << "Appending leaf with i = " << i << endl;
        forest.appendLeaf(NodeFactory::makeNode(new int(i)));

        auto oldRoots = forest.getOldRoots(i - 1);
        logdbg << "Fetched " << oldRoots.size() << " old roots for old forest with " << i-1 <<  " leaves..." << endl;
        assertTrue(oldRoots == expectedRoots);
        expectedRoots = forest.getRoots();
    }
}
