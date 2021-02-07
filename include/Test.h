#pragma once

#include <string>

namespace Thorn {
	void runTests();
	void testUHCI();
	void initAHCI();
	void testAHCI();
	void testPS2Keyboard();
	void handleInput(std::string);
}
