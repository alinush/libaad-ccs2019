#include <aad/Configuration.h>

#include <aad/Library.h>

#include <xassert/XAssert.h>

using namespace libaad;
using namespace std;

int main(int argc, char *argv[])
{
    (void)argc; (void)argv;

    initialize(nullptr, 0);

    Fr a = Fr::random_element(), b = Fr::random_element(), c;

    cout << a << endl;
    cout << b << endl;
    cout << a + b << endl;
    cout << a * b << endl;

    std::cout << "Test '" << argv[0] << "' finished successfully" << std::endl;

    return 0;
}
