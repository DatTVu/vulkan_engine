#include <iostream>
#include <Application.h>

int main() {
	VRE::Application MyApplication;
	try {
		MyApplication.Run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}