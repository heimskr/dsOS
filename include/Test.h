#pragma once

#include <memory>
#include <string>

namespace Thorn::FS {
	struct Partition;
	namespace ThornFAT {
		class ThornFATDriver;
	}
}

namespace Thorn::AHCI {
	class Controller;
	class Port;
}

namespace Thorn::Device {
	struct AHCIDevice;
}

namespace Thorn {
	void runTests();
	void testUHCI();
	void initAHCI();
	void testAHCI();
	void testPS2Keyboard();
	void handleInput(std::string);

	struct InputContext {
		AHCI::Controller *controller = nullptr;
		AHCI::Port *port = nullptr;
		FS::Partition *partition = nullptr;
		Device::AHCIDevice *ahciDevice = nullptr;
		FS::ThornFAT::ThornFATDriver *driver = nullptr;
		size_t ahciIndex = -1;
	};

	void init(const std::vector<std::string> &, InputContext &);
	void select(const std::vector<std::string> &, InputContext &);
	void selectPartition(size_t partition_index, InputContext &);
	void list(const std::vector<std::string> &, InputContext &);
	void listGPT(const std::vector<std::string> &, InputContext &);
	void listAHCI(const std::vector<std::string> &, InputContext &);
	void make(const std::vector<std::string> &, InputContext &);
}
