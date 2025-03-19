#include <StormByte/network/address.hxx>
#include <StormByte/test_handlers.h>

#include <format>

using namespace StormByte::Network;

int main() {
    int result = 0;

    if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }
    return result;
}
