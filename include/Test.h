#pragma once

#include "hardware/Serial.h"
#include "device/Device.h"

#include <memory>
#include <string>

extern volatile bool looping;

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
		bool ahci = true;
		int idePort = -1;
		Device::DeviceBase *device = nullptr;
		FS::Partition *partition = nullptr;
		FS::ThornFAT::ThornFATDriver *driver = nullptr;
		std::string path = "/";
		Ports::port_t portBase = Serial::COM1;
	};

	extern InputContext mainContext;

	void sha1(const std::vector<std::string> &, InputContext &);
	void parseElf(const std::vector<std::string> &, InputContext &);
	void mode(const std::vector<std::string> &, InputContext &);
	void records(const std::vector<std::string> &, InputContext &);
	void printSize(const std::vector<std::string> &, InputContext &);
	void readSerial(const std::vector<std::string> &, InputContext &);
	void set(const std::vector<std::string> &, InputContext &);
	void bars(const std::vector<std::string> &, InputContext &);
	void writeSerial(const std::vector<std::string> &, InputContext &);
	void cd(const std::vector<std::string> &, InputContext &);
	void rmdir(const std::vector<std::string> &, InputContext &);
	void remove(const std::vector<std::string> &, InputContext &);
	void create(const std::vector<std::string> &, InputContext &);
	void write(const std::vector<std::string> &, InputContext &);
	void read(const std::vector<std::string> &, InputContext &);
	void mkdir(const std::vector<std::string> &, InputContext &);
	void ls(const std::vector<std::string> &, InputContext &);
	void find(const std::vector<std::string> &, InputContext &);
	// void findPCI(const std::vector<std::string> &, InputContext &);
	void init(const std::vector<std::string> &, InputContext &);
	void select(const std::vector<std::string> &, InputContext &);
	void selectPartition(size_t partition_index, InputContext &);
	void list(const std::vector<std::string> &, InputContext &);
	void listPorts(InputContext &);
	void listGPT(InputContext &);
	void listAHCI(InputContext &);
	void make(const std::vector<std::string> &, InputContext &);
}
