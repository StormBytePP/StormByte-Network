#include <StormByte/network/address.hxx>
#include <StormByte/test_handlers.h>

#include <format>

using namespace StormByte::Network;

int test_localhost_address() {
	int result = 0;
	Address address("localhost", 80);
	if (!address.IsValid()) {
		std::cerr << std::format("Hostname {} is not valid or can't be resolved", address.Host()) << std::endl;
		result++;
	}
	
	RETURN_TEST("test_localhost_address", result);
}

int test_google_address() {
	int result = 0;
	Address address("google.com", 80);
	if (!address.IsValid()) {
		std::cerr << std::format("Hostname {} is not valid or can't be resolved", address.Host()) << std::endl;
		result++;
	}
	
	RETURN_TEST("test_localhost_address", result);
}

int test_ip_address() {
	int result = 0;
	Address address("127.0.0.1", 80);
	if (!address.IsValid()) {
		std::cerr << std::format("Hostname {} is not valid or can't be resolved", address.Host()) << std::endl;
		result++;
	}
	
	RETURN_TEST("test_localhost_address", result);
}

int main() {
    int result = 0;
	result += test_localhost_address();
	result += test_google_address();
	result += test_ip_address();

    if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << result << " tests failed." << std::endl;
    }
    return result;
}
