#include "Kernel.h"
#include "Terminal.h"
#include "Test.h"
#include "ThornUtil.h"
#include "device/AHCIDevice.h"
#include "device/IDEDevice.h"
#include "fs/ThornFAT/ThornFAT.h"
#include "fs/Partition.h"
#include "hardware/AHCI.h"
#include "hardware/IDE.h"
#include "hardware/GPT.h"
#include "hardware/MBR.h"
#include "hardware/PCI.h"
#include "hardware/Ports.h"
#include "hardware/PS2Keyboard.h"
#include "hardware/SATA.h"
#include "hardware/Serial.h"
#include "hardware/UHCI.h"
#include "memory/memset.h"
#include "multiboot2.h"
#include "arch/x86_64/APIC.h"
#include "arch/x86_64/control_register.h"
#include "arch/x86_64/CPU.h"
#include "arch/x86_64/Interrupts.h"
#include "arch/x86_64/PIC.h"
#include "lib/printf.h"

namespace Thorn {
	void runTests() {
		// testUHCI();
		// initAHCI();
		// testAHCI();
		testPS2Keyboard();
	}

	void testUHCI() {
		if (!UHCI::controllers) {
			printf("UHCI not initiated.\n");
		} else if (UHCI::controllers->empty()) {
			printf("No UHCI controllers found.\n");
		} else {
			printf("Found %d UHCI controller%s.\n", UHCI::controllers->size(), UHCI::controllers->size() == 1? "" : "s");
			UHCI::Controller &controller = UHCI::controllers->front();
			printf("First controller: %x:%x:%x\n", controller.device->bdf.bus, controller.device->bdf.device, controller.device->bdf.function);
			controller.init();

			using namespace Thorn::Ports;

			uint32_t bar4 = controller.address;

			// for (int i = 0; i <= 0xff; ++i)
			// 	printf("%x (%x): %x\n", i, bar4 + i, inb(bar4 + i));
			// 	printf("%x: %x\n", i, controller.device->readByte(i) & 0xff);

			printf("0x34: %x\n", controller.device->readByte(34) & 0xff);

			printf("0 (command): %x\n", inw(bar4 + 0));
			printf("2 (status): %x\n", inw(bar4 + 2));
			printf("4 (interrupts): %x\n", inw(bar4 + 4));
			printf("6 (frame #): %x\n", inw(bar4 + 6));
			printf("8 (frame base): %x\n", inl(bar4 + 8));
			printf("c (SoF): %x\n", inb(bar4 + 0xc));
			printf("0xc: %x\n", inb(bar4 + 0xc));
			printf("0x10: %x\n", inw(bar4 + 0x10));
			printf("0x12: %x\n", inw(bar4 + 0x12));
			printf("0x34: %x\n", inb(bar4 + 0x34));
			printf("portstatus(0) = 0x%x\n", controller.portStatus(0));
			printf("portstatus(1) = 0x%x\n", controller.portStatus(1));
			printf("Port count: %d\n", controller.countPorts());
		}
	}

	void initAHCI() {
		AHCI::controllers.clear();

		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t device = 0; device < 32; ++device)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = PCI::getVendorID(bus, device, function);
					if (vendor == PCI::INVALID_VENDOR)
						continue;

					const uint32_t baseclass = PCI::getBaseClass(bus, device, function);
					const uint32_t subclass  = PCI::getSubclass(bus, device, function);
					if (baseclass != 1 || subclass != 6)
						continue;

					AHCI::controllers.push_back(AHCI::Controller(PCI::initDevice({bus, device, function})));
					AHCI::Controller &controller = AHCI::controllers.back();
					if (!Kernel::instance) {
						printf("[initAHCI] Kernel instance is null!\n");
						return;
					}
					controller.init(*Kernel::instance);
					volatile AHCI::HBAMemory *abar = controller.abar;
					printf("Controller at %x:%x:%x (abar = 0x%llx):", bus, device, function, abar);
					printf(" %dL.%dP ", controller.device->getInterruptLine(), controller.device->getInterruptPin());
					printf(" cap=%b", abar->cap);
					printf("\n");

					for (int i = 0; i < 32; ++i) {
						volatile AHCI::HBAPort &port = abar->ports[i];
						if (port.clb == 0)
							continue;

						controller.ports[i] = new AHCI::Port(&port, abar);
						controller.ports[i]->init();
						ATA::DeviceInfo info;
						controller.ports[i]->identify(info);

						char model[41];
						info.copyModel(model);
						printf("Model: \"%s\"\n", model);
						printf("%d done.\n", i);
					}
				}
		printf("Finished initializing AHCI.\n");
	}

	void testAHCI() {
		if (AHCI::controllers.empty()) {
			printf("[testAHCI] No AHCI controllers found.\n");
			return;
		}

		AHCI::Controller &last_controller = AHCI::controllers.back();

		MBR mbr;
		AHCI::Port *port = nullptr;
		for (int i = 0; i < 32; ++i)
			if (last_controller.ports[i]) {
				port = last_controller.ports[i];
				break;
			}
		if (port) {
			port->read(0, 512, &mbr);
			if (mbr.indicatesGPT()) {
				GPT::Header gpt;
				port->readBytes(sizeof(GPT::Header), AHCI::Port::BLOCKSIZE, &gpt);
				printf("Signature:   0x%lx\n", gpt.signature);
				printf("Revision:    %d\n", gpt.revision);
				printf("Header size: %d\n", gpt.headerSize);
				printf("Current LBA: %ld\n", gpt.currentLBA);
				printf("Other LBA:   %ld\n", gpt.otherLBA);
				printf("First LBA:   %ld\n", gpt.firstLBA);
				printf("Last LBA:    %ld\n", gpt.lastLBA);
				printf("Start LBA:   %ld\n", gpt.startLBA);
				printf("Partitions:  %d\n", gpt.partitionCount);
				printf("Entry size:  %d\n", gpt.partitionEntrySize);
				size_t offset = AHCI::Port::BLOCKSIZE * gpt.startLBA;
				gpt.guid.print(true);
				if (gpt.partitionEntrySize != sizeof(GPT::PartitionEntry)) {
					printf("Unsupported partition entry size.\n");
				} else {
					GPT::PartitionEntry first_entry;
					for (unsigned i = 0; i < gpt.partitionCount; ++i) {
						GPT::PartitionEntry entry;
						port->readBytes(gpt.partitionEntrySize, offset, &entry);
						if (entry.typeGUID) {
							printf("Partition %d: \"", i);
							entry.printName(false);
							printf("\", type GUID[%s], partition GUID[%s], first[%ld], last[%ld]\n",
								std::string(entry.typeGUID).c_str(), std::string(entry.partitionGUID).c_str(),
								entry.firstLBA, entry.lastLBA);
							if (!first_entry.typeGUID)
								first_entry = entry;
						}
						offset += gpt.partitionEntrySize;
					}

					if (first_entry.typeGUID) {
						printf("Using partition \"%s\".\n", std::string(first_entry).c_str());
						Device::AHCIDevice device(port);
						FS::Partition partition(&device, first_entry.firstLBA * AHCI::Port::BLOCKSIZE,
							(first_entry.lastLBA - first_entry.firstLBA + 1) * AHCI::Port::BLOCKSIZE);
						using namespace FS::ThornFAT;
						auto driver = std::make_unique<ThornFATDriver>(&partition);
						driver->make(sizeof(DirEntry) * 5);
						printf("Made a ThornFAT partition.\n");
						int status;

						printf("\e[32;1;4mFirst readdir.\e[0m\n");
						status = driver->readdir("/", [](const char *path, off_t offset) {
							printf("\"%s\" @ %ld\n", path, offset); });
						if (status != 0) printf("readdir failed: %s\n", strerror(-status));

						printf("\e[32;1;4mCreating foo.\e[0m\n");
						status = driver->create("/foo", 0666);
						if (status != 0) printf("create failed: %s\n", strerror(-status));

						printf("\e[32;1;4mReaddir after creating foo.\e[0m\n");
						status = driver->readdir("/", [](const char *path, off_t offset) {
							printf("\"%s\" @ %ld\n", path, offset); });
						if (status != 0) printf("readdir failed: %s\n", strerror(-status));

						printf("\e[32;1;4mCreating bar.\e[0m\n");
						status = driver->create("/bar", 0666);
						if (status != 0) printf("create failed: %s\n", strerror(-status));

						printf("\e[32;1;4mReaddir after creating bar.\e[0m\n");
						status = driver->readdir("/", [](const char *path, off_t offset) {
							printf("\"%s\" @ %ld\n", path, offset); });
						if (status != 0) printf("readdir failed: %s\n", strerror(-status));

						printf("\e[32;1;4mDone.\e[0m\n");
						driver->superblock.print();
					}
				}
			}
		} else printf(":[\n");
	}

	void testPS2Keyboard() {
		std::string text;
		size_t index = 0;

		for (;;) {
			asm volatile("hlt");
			// Let's hope a keyboard interrupt doesn't occur here.
			while (!scancodes_fifo.empty()) {
				uint8_t scancode = scancodes_fifo.front();
				scancodes_fifo.pop();
				if (scancode == 0)
					continue;
				Keyboard::InputKey key = PS2Keyboard::scanmapNormal[scancode & ~0x80].key;
				bool down = (scancode & 0x80) == 0;
				if (key == Keyboard::InputKey::Invalid)
					scancode &= 0x80;

				Keyboard::onKey(key, (scancode & 0x80) == 0);
				key = Keyboard::transform(key);

				if (down) {
					switch (key) {
						case Keyboard::InputKey::KeyLeftArrow:
							if (0 < index) {
								Terminal::left();
								--index;
							}
							break;
						case Keyboard::InputKey::KeyRightArrow:
							if (index < text.size()) {
								Terminal::right();
								++index;
							}
							break;
						case Keyboard::InputKey::KeyBackspace:
							if (0 < index) {
								Terminal::left();
								printf(" ");
								Terminal::left();
								text.erase(--index);
							}
							break;
						case Keyboard::InputKey::KeyEnter:
							Terminal::clear();
							Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::Green, Terminal::VGAColor::Black);
							printf("> ");
							Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::LightGray, Terminal::VGAColor::Black);
							if (!Keyboard::hasModifier(Keyboard::Modifier::Shift)) {
								handleInput(text);
								text.clear();
								index = 0;
							}
							printf("%s", text.c_str());
							Terminal::row = (2 + text.size()) / Terminal::VGA_WIDTH;
							Terminal::column = (2 + text.size()) % Terminal::VGA_WIDTH;
							break;
						case Keyboard::InputKey::KeyLeftShift:
						case Keyboard::InputKey::KeyRightShift:
						case Keyboard::InputKey::KeyLeftAlt:
						case Keyboard::InputKey::KeyRightAlt:
						case Keyboard::InputKey::KeyLeftCtrl:
						case Keyboard::InputKey::KeyRightCtrl:
						case Keyboard::InputKey::KeyLeftMeta:
						case Keyboard::InputKey::KeyRightMeta:
						case Keyboard::InputKey::Invalid:
							break;
						default:
							text.insert(index++, Keyboard::toString(key).substr(0, 1));
							printf("%c", Keyboard::toString(key).front());
					}
				}
			}

#if 0
			if (scancode == (0x2c | 0x80)) { // z
				last_scancode = 0;
				printf("Hello!\n");
				std::string str(50000, 'A');
				printf("[%s:%d]\n", __FILE__, __LINE__);
				printf("(");
				for (char &ch: str) {
					if (ch != 'A') {
						printf_putc = false;
						printf("[%d 0x%lx]\n", ch, &ch);
						printf_putc = true;
					} else
						printf("%c", ch);
				}
				printf(")\n");
				printf("[%s:%d]\n", __FILE__, __LINE__);
			} else if (scancode == (0x2b | 0x80)) { // backslash
				// last_scancode = 0;
				// char buffer[2048] = {0};
				// printf(":: 0x%lx\n", &irqInvoked);

				// printf_putc = false;
				// for (int sector = 0; sector < 5; ++sector) {
				// 	printf("(%d)\n", IDE::readSectors(1, 1, sector, buffer));
				// 	for (size_t i = 0; i < sizeof(buffer); ++i)
				// 		printf("%c", buffer[i]);
				// 	printf("\n----------------------------\n");
				// 	memset(buffer, 0, sizeof(buffer));
				// }
				// printf_putc = true;
				// printf("\"%s\"\n", buffer);
			}
#endif
		}
	}

	void handleInput(std::string str) {
		if (str.empty())
			return;

		const size_t first_non_space = str.find_first_not_of(" ");
		if (first_non_space == std::string::npos)
			return;

		if (first_non_space)
			str = str.substr(first_non_space);

		while (!str.empty() && str.back() == ' ')
			str.pop_back();

		std::vector<std::string> pieces = Util::split(str, " ", true);

		printf("\n");

		static InputContext context;

		if (pieces[0] == "hello") {
			printf("How are you?\n");
		} else if (pieces[0] == "list") {
			list(pieces, context);
		} else if (pieces[0] == "init") {
			init(pieces, context);
		} else if (pieces[0] == "select" || pieces[0] == "sel") {
			select(pieces, context);
		} else if (pieces[0] == "make") {
			make(pieces, context);
		} else {
			printf("Unknown command.\n");
		}
	}

	void init(const std::vector<std::string> &pieces, InputContext &) {
		if (pieces.size() < 2) {
			printf("Not enough arguments.\n");
		} else if (pieces[1] == "ahci") {
			initAHCI();
		}
	}

	void select(const std::vector<std::string> &pieces, InputContext &context) {
		auto usage = [] { printf("Usage:\n- select controller <#>\n- select port <#>\n- select partition <#>\n"); };

		if (pieces.size() == 3) {
			if (pieces[1] == "controller") {
				size_t controller_index;
				if (!Util::parseUlong(pieces[2], controller_index)) {
					usage();
				} else if (AHCI::controllers.size() <= controller_index) {
					printf("Controller index out of range.\n");
				} else {
					context.controller = &AHCI::controllers[controller_index];
					context.port = nullptr;
					if (context.partition)
						delete context.partition;
					context.partition = nullptr;
					if (context.driver)
						delete context.driver;
					context.driver = nullptr;
					printf("Selected controller %lu.\n", controller_index);
				}
			} else if (pieces[1] == "port") {
				if (!context.controller) {
					printf("No controller is selected.\n");
					return;
				}

				AHCI::Controller &controller = *context.controller;
				size_t port_index;
				if (!Util::parseUlong(pieces[2], port_index)) {
					usage();
				} else if (32 <= port_index) {
					printf("Port index out of range.\n");
				} else if (!controller.ports[port_index]) {
					printf("Invalid port.\n");
				} else {
					context.port = controller.ports[port_index];
					printf("Selected port %lu.\n", port_index);
				}
			} else if (pieces[1] == "part" || pieces[1] == "partition") {
				if (!context.port) {
					printf("No port selected.\n");
				} else {
					size_t partition_index;
					if (!Util::parseUlong(pieces[2], partition_index))
						printf("Unable to parse partition index: %s\n", pieces[2].c_str());
					else
						selectPartition(partition_index, context);
				}
			} else {
				usage();
			}
		} else {
			usage();
		}
	}

	void selectPartition(size_t partition_index, InputContext &context) {
		MBR mbr;
		context.port->read(0, 512, &mbr);
		if (!mbr.indicatesGPT()) {
			printf("MBR doesn't indicate the presence of a GPT.\n");
			return;
		}

		GPT::Header gpt;
		context.port->readBytes(sizeof(GPT::Header), AHCI::Port::BLOCKSIZE, &gpt);
		if (gpt.partitionEntrySize != sizeof(GPT::PartitionEntry)) {
			printf("Unsupported partition entry size.\n");
			return;
		}
		size_t offset = AHCI::Port::BLOCKSIZE * gpt.startLBA + gpt.partitionEntrySize * partition_index;
		GPT::PartitionEntry entry;
		context.port->readBytes(gpt.partitionEntrySize, offset, &entry);
		if (!entry.typeGUID) {
			printf("Invalid partition.\n");
			return;
		}

		if (context.ahciDevice)
			delete context.ahciDevice;

		context.ahciDevice = new Device::AHCIDevice(context.port);
		context.partition = new FS::Partition(context.ahciDevice, entry.firstLBA * AHCI::Port::BLOCKSIZE,
				(entry.lastLBA - entry.firstLBA + 1) * AHCI::Port::BLOCKSIZE);
		printf("Selected partition \"%s\".\n", std::string(entry).c_str());
	}

	void list(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() < 2) {
			printf("Not enough arguments.\n");
		} else if (pieces[1] == "ahci") {
			listAHCI(pieces, context);
		} else if (pieces[1] == "gpt") {
			listGPT(pieces, context);
		} else if (pieces[1] == "port" || pieces[1] == "ports") {
			listPorts(pieces, context);
		} else {
			printf("Usage:\n- list ahci\n- list gpt\n- list port\n");
		}
	}

	void listPorts(const std::vector<std::string> &pieces, InputContext &context) {
		if (!context.controller) {
			printf("No controller is selected.\n");
			return;
		}

		for (int i = 0; i < 32; ++i) {
			if (context.controller->ports[i]) {
				printf("[%d] %s: \"", i, AHCI::deviceTypes[static_cast<int>(context.controller->ports[i]->type)]);
				ATA::DeviceInfo info;
				context.controller->ports[i]->identify(info);
				char model[41];
				info.copyModel(model);
				printf("%s\"\n", model);
			}
		}
	}

	void listGPT(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 4) {
			printf("Usage: list gpt <controller> <port>\n");
			return;
		}

		size_t controller_index;
		if (!Util::parseUlong(pieces[2], controller_index) || AHCI::controllers.size() <= controller_index) {
			printf("Invalid controller index: %s\n", pieces[2].c_str());
			return;
		}

		AHCI::Controller &controller = AHCI::controllers[controller_index];
		size_t port_index;
		if (!Util::parseUlong(pieces[3], port_index) || !controller.ports[port_index]) {
			printf("Invalid port index: %s\n", pieces[3].c_str());
			return;
		}

		AHCI::Port *port = controller.ports[port_index];
		MBR mbr;
		port->read(0, 512, &mbr);
		if (!mbr.indicatesGPT()) {
			printf("MBR doesn't indicate the presence of a GPT.\n");
			return;
		}

		GPT::Header gpt;
		port->readBytes(sizeof(GPT::Header), AHCI::Port::BLOCKSIZE, &gpt);
		printf("Signature:   0x%lx\n", gpt.signature);
		printf("Revision:    %d\n",    gpt.revision);
		printf("Header size: %d\n",    gpt.headerSize);
		printf("Current LBA: %ld\n",   gpt.currentLBA);
		printf("Other LBA:   %ld\n",   gpt.otherLBA);
		printf("First LBA:   %ld\n",   gpt.firstLBA);
		printf("Last LBA:    %ld\n",   gpt.lastLBA);
		printf("Start LBA:   %ld\n",   gpt.startLBA);
		printf("Partitions:  %d\n",    gpt.partitionCount);
		printf("Entry size:  %d\n",    gpt.partitionEntrySize);
		size_t offset = AHCI::Port::BLOCKSIZE * gpt.startLBA;
		printf("GUID:        %s\n", std::string(gpt.guid).c_str());
		if (gpt.partitionEntrySize != sizeof(GPT::PartitionEntry)) {
			printf("Unsupported partition entry size.\n");
			return;
		}

		for (unsigned i = 0; i < gpt.partitionCount; ++i) {
			GPT::PartitionEntry entry;
			port->readBytes(gpt.partitionEntrySize, offset, &entry);
			if (entry.typeGUID) {
				printf("Partition %d: \"", i);
				entry.printName(false);
				printf("\"\n  Type GUID: %s\n  Partition GUID: %s\n  First = %ld, last = %ld\n",
					std::string(entry.typeGUID).c_str(), std::string(entry.partitionGUID).c_str(), entry.firstLBA,
					entry.lastLBA);
			}
			offset += gpt.partitionEntrySize;
		}
	}

	void listAHCI(const std::vector<std::string> &pieces, InputContext &) {
		if (pieces.size() == 2) {
			if (AHCI::controllers.empty()) {
				printf("No AHCI controllers found.\n");
			} else {
				printf("AHCI controllers:\n", AHCI::controllers.size());
				size_t i = 0;
				for (AHCI::Controller &controller: AHCI::controllers) {
					const auto &bdf = controller.device->bdf;
					printf("[%d] %x:%x:%x: ports =", i, bdf.bus, bdf.device, bdf.function);
					for (int i = 0; i < 32; ++i)
						if (controller.ports[i])
							printf(" %d", i);
					printf("\n");
					++i;
				}
				printf("Done.\n");
			}
		} else if (pieces.size() == 3) {
			long index;
			if (!Util::parseLong(pieces[2], index)) {
				printf("Invalid number: %s\n", pieces[2].c_str());
			} else if (AHCI::controllers.size() <= static_cast<size_t>(index)) {
				printf("Invalid index: %ld\n", index);
			} else {
				AHCI::Controller &controller = AHCI::controllers[index];
				for (int i = 0; i < 32; ++i) {
					if (controller.ports[i]) {
						printf("[%d] %s: \"", i, AHCI::deviceTypes[static_cast<int>(controller.ports[i]->type)]);
						ATA::DeviceInfo info;
						controller.ports[i]->identify(info);
						char model[41];
						info.copyModel(model);
						printf("%s\"\n", model);
					}
				}
			}
		} else {
			printf("Usage:\n- list ahci\n- list ahci <controller>\n");
		}
	}

	void make(const std::vector<std::string> &, InputContext &context) {
		if (!context.driver) {
			printf("Error: Driver isn't ready.\n");
		} else {
			context.driver->make(sizeof(FS::ThornFAT::DirEntry) * 5);
		}
	}
}
