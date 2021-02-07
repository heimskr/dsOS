#pragma once

#include <string>

namespace Thorn::FS {
	struct Partition;
	namespace ThornFAT {
		class ThornFATDriver;
	}
}

namespace Thorn::AHCI {
	class Controller;
}

namespace Thorn {
	void runTests();
	void testUHCI();
	void initAHCI();
	void testAHCI();
	void testPS2Keyboard();
	void handleInput(std::string);

	struct InputContext {
		FS::Partition *partition = nullptr;
		FS::ThornFAT::ThornFATDriver *driver = nullptr;
		AHCI::Controller *controller = nullptr;
		size_t ahciIndex = -1;
	};

	void init(const std::vector<std::string> &, InputContext &);
	void list(const std::vector<std::string> &, InputContext &);
	void listAHCI(const std::vector<std::string> &, InputContext &);
	void make(const std::vector<std::string> &, InputContext &);
}
