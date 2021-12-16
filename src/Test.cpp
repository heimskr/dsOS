#include "Kernel.h"
#include "Terminal.h"
#include "Test.h"
#include "ThornUtil.h"
#include "device/AHCIDevice.h"
#include "device/IDEDevice.h"
#include "fs/ThornFAT/ThornFAT.h"
#include "fs/Partition.h"
#include "fs/Util.h"
#include "hardware/AHCI.h"
#include "hardware/IDE.h"
#include "hardware/GPT.h"
#include "hardware/MBR.h"
#include "hardware/PCI.h"
#include "hardware/PCIIDs.h"
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
#include "lib/ElfParser.h"
#include "lib/printf.h"
#include "lib/SHA1.h"

volatile bool looping = false;

namespace Thorn {
	void runTests() {
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
					AHCI::HBAMemory *abar = controller.abar;
					printf("Controller at %x:%x:%x (abar = 0x%llx):", bus, device, function, abar);
					printf(" %dL.%dP ", controller.device->getInterruptLine(), controller.device->getInterruptPin());
					printf(" cap=%b", abar->cap);
					printf("\n");

					for (int i = 0; i < 32; ++i) {
						AHCI::HBAPort &port = abar->ports[i];
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
								tprintf(" ");
								Terminal::left();
								text.erase(--index);
							}
							break;
						case Keyboard::InputKey::KeyEnter:
							Terminal::clear();
							Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::Green, Terminal::VGAColor::Black);
							tprintf("> ");
							Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::LightGray, Terminal::VGAColor::Black);
							if (!Keyboard::hasModifier(Keyboard::Modifier::Shift)) {
								handleInput(text);
								text.clear();
								index = 0;
							}
							tprintf("%s", text.c_str());
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
						case Keyboard::InputKey::KeyCapsLock:
						case Keyboard::InputKey::Invalid:
							break;
						default:
							text.insert(index++, Keyboard::toString(key).substr(0, 1));
							tprintf("%c", Keyboard::toString(key).front());
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

	InputContext mainContext;

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

		tprintf("\n");

		if (pieces[0] == "hello") {
			tprintf("How are you?\n");
		} else if (pieces[0] == "list") {
			list(pieces, mainContext);
		} else if (pieces[0] == "init") {
			init(pieces, mainContext);
		} else if (pieces[0] == "select" || pieces[0] == "sel") {
			select(pieces, mainContext);
		} else if (pieces[0] == "make") {
			make(pieces, mainContext);
		} else if (pieces[0] == "find") {
			find(pieces, mainContext);
		} else if (pieces[0] == "ls") {
			ls(pieces, mainContext);
		} else if (pieces[0] == "mkdir") {
			mkdir(pieces, mainContext);
		} else if (pieces[0] == "read") {
			read(pieces, mainContext);
		} else if (pieces[0] == "write") {
			write(pieces, mainContext);
		} else if (pieces[0] == "create") {
			create(pieces, mainContext);
		} else if (pieces[0] == "rm") {
			remove(pieces, mainContext);
		} else if (pieces[0] == "rmdir") {
			rmdir(pieces, mainContext);
		} else if (pieces[0] == "cd") {
			cd(pieces, mainContext);
		} else if (pieces[0] == "serial") {
			writeSerial(pieces, mainContext);
		} else if (pieces[0] == "set") {
			set(pieces, mainContext);
		} else if (pieces[0] == "readserial") {
			readSerial(pieces, mainContext);
		} else if (pieces[0] == "size") {
			printSize(pieces, mainContext);
		} else if (pieces[0] == "records") {
			records(pieces, mainContext);
		} else if (pieces[0] == "mode") {
			mode(pieces, mainContext);
		} else if (pieces[0] == "parseelf") {
			parseElf(pieces, mainContext);
		} else if (pieces[0] == "sha1") {
			sha1(pieces, mainContext);
		} else if (pieces[0] == "loop") {
			tprintf("Looping...\n");
			looping = true;
			while (looping) {
				Serial::write(0b11110000, mainContext.portBase);
			}
			tprintf("Done.\n");
		} else if (pieces[0] == "0") {
			handleInput("init ahci");
			handleInput("sel cont 0");
			handleInput("sel port 0");
			handleInput("sel part 0");
			handleInput("init tfat");
		} else if (pieces[0] == "0i") {
			handleInput("mode ide");
			handleInput("init ide");
			handleInput("sel port 0");
			handleInput("sel part 0");
			handleInput("init tfat");
		} else if (pieces[0] == "printfat") {
			if (!mainContext.driver) printf("Driver isn't ready.\n");
			else if (!mainContext.driver->verify()) printf("Driver couldn't verify filesystem validity.\n");
			else {
				size_t count = 100;
				if (1 < pieces.size())
					Util::parseUlong(pieces[1], count);
				for (size_t i = 0; i < count; ++i)
					printf("%lu -> %d\n", i, mainContext.driver->readFAT(i));
			}
		} else
			tprintf("Unknown command.\n");
	}

	void sha1(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() == 3) {
			CSHA1 sha1;
			sha1.Update((uint8_t *) pieces[2].c_str(), pieces[2].size());
			sha1.Final();
			std::string hash;
			sha1.ReportHashStl(hash, CSHA1::REPORT_HEX_SHORT);
			printf("Result: %s\n", hash.c_str());
		} else if (pieces.size() != 2) {
			tprintf("Usage:\n- sha1 <path>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			size_t size;
			int status = context.driver->getsize(path.c_str(), size);
			if (status != 0) {
				tprintf("Couldn't read filesize: %s (%d)\n", strerror(-status), status);
				return;
			}

			if (size == 0) {
				tprintf("File is empty.\n");
				return;
			}

			char *buffer = new char[size];
			status = context.driver->read(path.c_str(), buffer, size, 0);
			if (status < 0) {
				tprintf("Couldn't read file: %s (%d)\n", strerror(-status), status);
				return;
			}

			CSHA1 sha1;
			sha1.Update((const unsigned char *) buffer, size);
			sha1.Final();
			std::string hash;
			sha1.ReportHashStl(hash, CSHA1::REPORT_HEX_SHORT);
			printf("Result: %s\n", hash.c_str());
			delete[] buffer;
		}
	}

	void parseElf(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 2) {
			tprintf("Usage:\n- parseelf <path>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			size_t size;
			int status = context.driver->getsize(path.c_str(), size);
			if (status != 0) {
				tprintf("Couldn't read filesize: %s (%d)\n", strerror(-status), status);
				return;
			}

			if (size == 0) {
				tprintf("File is empty.\n");
				return;
			}

			char *buffer = new char[size];
			status = context.driver->read(path.c_str(), buffer, size, 0);
			if (status < 0) {
				tprintf("Couldn't read file: %s (%d)\n", strerror(-status), status);
				return;
			}

			Elf::ElfParser parser(buffer);

			for (const Elf::Section &section: parser.getSections()) {
				printf("Section[index=%d, offset=0x%lx, addr=0x%lx, name=\"%s\", type=\"%s\", size=%d, entsize=%d, "
					"addralign=%d]\n", section.index, section.offset, section.addr, section.name.c_str(),
					section.type.c_str(), section.size, section.entSize, section.addrAlign);
			}

			delete[] buffer;
		}
	}

	void mode(const std::vector<std::string> &pieces, InputContext &context) {
		auto usage = [] { tprintf("Usage:\n- mode ahci\n- mode ide\n"); };
		if (pieces.size() != 2) {
			usage();
		} else if (pieces[1] == "ahci") {
			if (context.ahci) {
				tprintf("Already in AHCI mode.\n");
				return;
			}

			context.ahci = true;

		} else if (pieces[1] == "ide") {
			if (!context.ahci) {
				tprintf("Already in IDE mode.\n");
				return;
			}

			context.ahci = false;
		} else {
			usage();
			return;
		}

		if (context.driver) {
			delete context.driver;
			context.driver = nullptr;
		}

		if (context.partition) {
			delete context.partition;
			context.partition = nullptr;
		}

		if (context.port) {
			delete context.port;
			context.port = nullptr;
		}

		if (context.controller) {
			delete context.controller;
			context.controller = nullptr;
		}

		context.path = "/";
		context.idePort = -1;

		tprintf("Switched to %s mode.\n", context.ahci? "AHCI" : "IDE");
	}

	void records(const std::vector<std::string> &pieces, InputContext &context) {
		auto usage = [] { tprintf("Usage:\n- records write\n- records read\n"); };
		if (!context.partition)
			tprintf("Partition is invalid.\n");
		else if (pieces.size() != 2)
			usage();
		else if (pieces[1] == "write")
			for (const auto &record: context.partition->writeRecords)
				printf("[s=%lu, o=%lu]\n", record.size, record.offset);
		else if (pieces[1] == "read")
			for (const auto &record: context.partition->readRecords)
				printf("[s=%lu, o=%lu]\n", record.size, record.offset);
		else
			usage();
	}

	void printSize(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 2) {
			tprintf("Usage:\n- size <file>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			size_t size;
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			int status = context.driver->getsize(path.c_str(), size);
			if (status != 0)
				tprintf("Couldn't check filesize: %s (%d)\n", strerror(-status), status);
			else
				tprintf("Filesize of %s: %lu\n", path.c_str(), size);
		}
	}

	void readSerial(const std::vector<std::string> &pieces, InputContext &context) {
		if (2 < pieces.size()) {
			tprintf("Usage:\n- readserial [file]\n");
			return;
		}

		std::string path;

		if (pieces.size() == 2) {
			if (!context.driver) {
				tprintf("Driver isn't ready.\n");
				return;
			} else if (!context.driver->verify()) {
				tprintf("Driver couldn't verify filesystem validity.\n");
				return;
			} else {
				path = FS::simplifyPath(context.path, pieces[1]);
				int status = context.driver->isfile(path.c_str());
				if (status < 0) {
					tprintf("Couldn't check for file: %s (%d)\n", strerror(-status), status);
					return;
				} else if (status == 0) {
					tprintf("%s is a directory.\n", path.c_str());
					return;
				}
			}
		}

		std::string input;
		for (;;) {
			char ch = Serial::read(context.portBase);
			if (ch == '^') {
				char next = Serial::read(context.portBase);
				if (next == '^')
					input += ch;
				else if (next == 'q')
					break;
			} else
				input += ch;
		}

		if (path.empty()) {
			printf("Serial input: [");
			for (size_t i = 0; i < input.size(); ++i)
				printf("%c", input[i]);
			printf("]\n");
			return;
		}

		int status = context.driver->truncate(path.c_str(), input.size());
		if (status < 0) {
			tprintf("Couldn't truncate file: %s (%d)\n", strerror(-status), status);
			return;
		}

		status = context.driver->write(path.c_str(), input.c_str(), input.size(), 0);
		if (status < 0)
			tprintf("Couldn't write to %s: %s (%d)\n", path.c_str(), strerror(-status), status);
		else
			tprintf("Wrote %lu byte%s to %s.\n", input.size(), input.size() == 1? "" : "s", path.c_str());
	}

	void set(const std::vector<std::string> &pieces, InputContext &context) {
		auto usage = [] { tprintf("Usage:\n- set baseport <baseport>\n"); };
		if (pieces.size() < 2) {
			usage();
		} else if (pieces[1] == "baseport") {
			if (pieces.size() != 3) {
				usage();
			} else {
				long baseport;
				if (2 < pieces[2].size() && pieces[2].substr(0, 2) == "0x") {
					if (!Util::parseLong(pieces[2].substr(2), baseport, 16)) {
						tprintf("Invalid baseport.\n");
						return;
					}
				} else if (!Util::parseLong(pieces[2], baseport)) {
					tprintf("Invalid baseport.\n");
					return;
				}

				context.portBase = baseport;
				tprintf("baseport set to 0x%x.\n", static_cast<Ports::port_t>(baseport));
			}
		} else {
			usage();
		}
	}

	void writeSerial(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() < 2) {
			tprintf("Usage:\n- serial <text...>\n");
			return;
		}

		Serial::ready = false;
		if (!Serial::init(context.portBase)) {
			tprintf("Serial failed to initialize for base 0x%x.\n", context.portBase);
		} else {
			std::string text = pieces[1];
			for (size_t i = 2; i < pieces.size(); ++i)
				text += " " + pieces[i];
			Serial::write(text.c_str(), context.portBase);
			Serial::write('\n', context.portBase);
		}
	}

	void cd(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 2) {
			tprintf("Usage:\n- cd <path>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			int status = context.driver->isdir(path.c_str());
			if (status == 0)
				tprintf("Can't change directory: path is a file\n");
			else if (status < 0)
				tprintf("Can't change directory: %s (%d)\n", strerror(-status), status);
			else
				context.path = path;
		}
	}

	void rmdir(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 2) {
			tprintf("Usage:\n- rmdir <path>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			int status = context.driver->rmdir(path.c_str());
			if (status != 0)
				tprintf("Couldn't remove directory: %s (%d)\n", strerror(-status), status);
			else
				tprintf("Removed %s.\n", path.c_str());
		}
	}

	void remove(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 2) {
			tprintf("Usage:\n- rm <path>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			int status = context.driver->unlink(path.c_str());
			if (status != 0)
				tprintf("Couldn't remove file: %s (%d)\n", strerror(-status), status);
			else
				tprintf("Removed %s.\n", path.c_str());
		}
	}

	void create(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 2 && pieces.size() != 3) {
			tprintf("Usage:\n- create <path> [modes]\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			size_t modes = 0644;

			if (pieces.size() == 3 && !Util::parseUlong(pieces[2], modes, 8)) {
				tprintf("Couldn't parse modes.\n");
				return;
			}

			int status = context.driver->create(path.c_str(), modes);
			if (status != 0)
				tprintf("Couldn't create file: %s (%d)\n", strerror(-status), status);
			else
				tprintf("Created %s.\n", path.c_str());
		}
	}

	void write(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() < 3) {
			tprintf("Usage:\n- write <path> <contents...>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			std::string joined = pieces[2];
			for (size_t i = 3; i < pieces.size(); ++i)
				joined += " " + pieces[i];

			int status = context.driver->truncate(path.c_str(), joined.size());
			if (status < 0) {
				tprintf("Couldn't truncate file: %s (%d)\n", strerror(-status), status);
			} else {
				status = context.driver->write(path.c_str(), joined.c_str(), joined.size(), 0);
				if (status < 0)
					tprintf("Couldn't write to file: %s (%d)\n", strerror(-status), status);
				else
					tprintf("Wrote %lu bytes to %s.\n", joined.size(), path.c_str());
			}
		}
	}

	void read(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 2) {
			tprintf("Usage:\n- read <path>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			size_t size;
			int status = context.driver->getsize(path.c_str(), size);
			if (status != 0) {
				tprintf("Couldn't read filesize: %s (%d)\n", strerror(-status), status);
				return;
			}

			if (size == 0) {
				tprintf("File is empty.\n");
				return;
			}

			char *buffer = new char[size];
			status = context.driver->read(path.c_str(), buffer, size, 0);
			if (status < 0) {
				tprintf("Couldn't read file: %s (%d)\n", strerror(-status), status);
				return;
			}

			printf("%s: [", path.c_str());
			for (size_t i = 0; i < size; ++i)
				printf("%c", buffer[i]);
			printf("]\n");

			delete[] buffer;
		}
	}

	void mkdir(const std::vector<std::string> &pieces, InputContext &context) {
		if (pieces.size() != 2) {
			tprintf("Usage:\n- mkdir <path>\n");
		} else if (!context.driver) {
			tprintf("Driver isn't ready.\n");
		} else if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
		} else {
			const std::string path = FS::simplifyPath(context.path, pieces[1]);
			int status = context.driver->mkdir(path.c_str(), 0777);
			if (status != 0)
				tprintf("mkdir failed: %s (%d)\n", strerror(-status), status);
		}
	}

	void ls(const std::vector<std::string> &pieces, InputContext &context) {
		if (!context.driver) {
			tprintf("Driver isn't ready.\n");
			return;
		}

		if (!context.driver->verify()) {
			tprintf("Driver couldn't verify filesystem validity.\n");
			return;
		}

		std::string path = context.path;

		if (pieces.size() == 2) {
			path = FS::simplifyPath(path, pieces[1]);
		} else if (pieces.size() != 1) {
			tprintf("Usage:\n- ls [path]\n");
			return;
		}

		int status = context.driver->readdir(path.c_str(), [](const char *item, off_t) { tprintf("%s\n", item); });
		if (status != 0)
			tprintf("readdir(%s) failed: %s (%d)\n", path.c_str(), strerror(-status), status);
	}

	void find(const std::vector<std::string> &pieces, InputContext &) {
		auto usage = [] {
			tprintf("Usage:\n- find pci [class] [subclass] [interface]\n- find bar <bus> <device> <function>\n");
		};
		if (pieces.size() < 2) {
			usage();
		} else if (pieces[1] == "pci") {
			if (5 < pieces.size()) {
				tprintf("Too many arguments.\n");
				usage();
				return;
			}

			long target_baseclass = -1, target_subclass = -1, target_interface = -1;
			if (3 <= pieces.size() && !Util::parseLong(pieces[2], target_baseclass)) {
				tprintf("Invalid class: %s\n", pieces[2].c_str());
				return;
			}
			if (4 <= pieces.size() && !Util::parseLong(pieces[3], target_subclass)) {
				tprintf("Invalid subclass: %s\n", pieces[3].c_str());
				return;
			}
			if (5 <= pieces.size() && !Util::parseLong(pieces[4], target_interface)) {
				tprintf("Invalid interface: %s\n", pieces[4].c_str());
				return;
			}

			bool color = false;
			Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::Magenta, Terminal::VGAColor::Black);

			for (uint32_t bus = 0; bus < 256; ++bus)
				for (uint32_t device = 0; device < 32; ++device)
					for (uint32_t function = 0; function < 8; ++function) {
						const uint32_t vendor = PCI::getVendorID(bus, device, function);
						if (vendor == PCI::INVALID_VENDOR || vendor == 0)
							continue;
						const uint32_t device_id = PCI::getDeviceID(bus, device, function);
						const uint32_t baseclass = PCI::getBaseClass(bus, device, function);
						const uint32_t subclass  = PCI::getSubclass(bus, device, function);
						const uint32_t interface = PCI::getProgIF(bus, device, function);
						if (target_baseclass != -1 && target_baseclass != baseclass)
							continue;
						if (target_subclass != -1 && target_subclass != subclass)
							continue;
						if (target_interface != -1 && target_interface != interface)
							continue;
						if (PCI::ID::IDSet *pci_ids = PCI::ID::getDeviceIDs(vendor, device_id, 0, 0)) {
							if (target_baseclass != -1 && target_subclass != -1 && target_interface != -1)
								tprintf("[%x:%x:%x] %s ", bus, device, function, pci_ids->deviceName);
							else
								tprintf("[%x:%x:%x] (%x/%x/%x) %s ", bus, device, function, baseclass, subclass,
									interface, pci_ids->deviceName);
							if (color)
								Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::Magenta, Terminal::VGAColor::Black);
							else
								Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::Cyan, Terminal::VGAColor::Black);
							color = !color;
						}
					}
			Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::LightGray, Terminal::VGAColor::Black);
		} else if (pieces[1] == "bar" && pieces.size() == 5) {
			long target_bus = -1, target_device = -1, target_function = -1;
			if (!Util::parseLong(pieces[2], target_bus)) {
				tprintf("Invalid bus: %s\n", pieces[2].c_str());
			} else if (!Util::parseLong(pieces[3], target_device)) {
				tprintf("Invalid device: %s\n", pieces[3].c_str());
			} else if (!Util::parseLong(pieces[4], target_function)) {
				tprintf("Invalid function: %s\n", pieces[4].c_str());
			} else {
				PCI::HeaderNative header = PCI::readNativeHeader({
					static_cast<unsigned>(target_bus),
					static_cast<unsigned>(target_device),
					static_cast<unsigned>(target_function)
				});
				printf("BAR0: 0x%x\n", header.bar0);
				printf("BAR1: 0x%x\n", header.bar1);
				printf("BAR2: 0x%x\n", header.bar2);
				printf("BAR3: 0x%x\n", header.bar3);
				printf("BAR4: 0x%x\n", header.bar4);
				printf("BAR5: 0x%x\n", header.bar5);
			}
		} else {
			usage();
		}
	}

	void init(const std::vector<std::string> &pieces, InputContext &context) {
		auto usage = [] { tprintf("Usage:\n- init ahci\n- init ide\n- init thornfat\n"); };
		if (pieces.size() < 2) {
			usage();
		} else if (pieces[1] == "ahci") {
			context.controller = nullptr;
			context.port = nullptr;
			if (context.partition)
				delete context.partition;
			context.partition = nullptr;
			if (context.device)
				delete context.device;
			context.device = nullptr;
			if (context.driver)
				delete context.driver;
			context.driver = nullptr;
			initAHCI();
		} else if (pieces[1] == "ide") {
			if (IDE::init() == 0)
				tprintf("No IDE devices found.\n");
		} else if (pieces[1] == "thornfat" || pieces[1] == "tfat" || pieces[1] == "driver") {
			if (!context.device || !context.partition)
				tprintf("No partition is selected.\n");
			else {
				context.driver = new FS::ThornFAT::ThornFATDriver(context.partition);
				tprintf("Initialized ThornFAT driver.\n");
			}
		} else
			usage();
	}

	void select(const std::vector<std::string> &pieces, InputContext &context) {
		auto usage = [] { tprintf("Usage:\n- select controller <#>\n- select port <#>\n- select partition <#>\n"); };

		if (pieces.size() == 3) {
			if (pieces[1] == "controller" || pieces[1] == "ahci" || pieces[1] == "cont") {
				if (!context.ahci) {
					tprintf("Can't select AHCI controller: not in AHCI mode.\n");
					return;
				}

				size_t controller_index;
				if (!Util::parseUlong(pieces[2], controller_index)) {
					usage();
				} else if (AHCI::controllers.size() <= controller_index) {
					tprintf("Controller index out of range.\n");
				} else {
					context.controller = &AHCI::controllers[controller_index];
					context.port = nullptr;
					if (context.partition)
						delete context.partition;
					context.partition = nullptr;
					if (context.driver)
						delete context.driver;
					context.driver = nullptr;
					tprintf("Selected controller %lu.\n", controller_index);
				}
			} else if (pieces[1] == "port") {
				if (context.ahci) {
					if (!context.controller) {
						tprintf("No controller is selected.\n");
						return;
					}

					AHCI::Controller &controller = *context.controller;
					size_t port_index;
					if (!Util::parseUlong(pieces[2], port_index)) {
						usage();
					} else if (32 <= port_index) {
						tprintf("Port index out of range.\n");
					} else if (!controller.ports[port_index]) {
						tprintf("Invalid port.\n");
					} else {
						context.port = controller.ports[port_index];
						tprintf("Selected port %lu.\n", port_index);
					}
				} else {
					size_t port_index;
					if (!Util::parseUlong(pieces[2], port_index)) {
						usage();
					} else if (4 <= port_index) {
						tprintf("Port index out of range.\n");
					} else {
						context.idePort = static_cast<int>(port_index);
						tprintf("Selected port %d.\n", context.idePort);
					}
				}
			} else if (pieces[1] == "part" || pieces[1] == "partition") {
				if ((context.ahci && !context.port) || (!context.ahci && context.idePort == -1)) {
					tprintf("No port selected.\n");
				} else {
					size_t partition_index;
					if (!Util::parseUlong(pieces[2], partition_index))
						tprintf("Unable to parse partition index: %s\n", pieces[2].c_str());
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
		if ((context.ahci && !context.port) || (!context.ahci && context.idePort == -1)) {
			tprintf("No port is selected.\n");
			return;
		}

		MBR mbr;
		if (context.ahci)
			context.port->read(0, 512, &mbr);
		else
			IDE::readBytes(context.idePort, 512, 0, &mbr);

		if (!mbr.indicatesGPT()) {
			tprintf("MBR doesn't indicate the presence of a GPT.\n");
			return;
		}

		const size_t bs = context.ahci? AHCI::Port::BLOCKSIZE : IDE::SECTOR_SIZE;

		GPT::Header gpt;

		if (context.ahci)
			context.port->readBytes(sizeof(GPT::Header), bs, &gpt);
		else
			IDE::readBytes(context.idePort, sizeof(GPT::Header), bs, &gpt);

		if (gpt.partitionEntrySize != sizeof(GPT::PartitionEntry)) {
			tprintf("Unsupported partition entry size.\n");
			return;
		}
		size_t offset = bs * gpt.startLBA + gpt.partitionEntrySize * partition_index;
		GPT::PartitionEntry entry;

		if (context.ahci)
			context.port->readBytes(gpt.partitionEntrySize, offset, &entry);
		else
			IDE::readBytes(context.idePort, gpt.partitionEntrySize, offset, &entry);

		if (!entry.typeGUID) {
			tprintf("Invalid partition.\n");
			return;
		}

		if (context.device)
			delete context.device;

		if (context.ahci)
			context.device = new Device::AHCIDevice(context.port);
		else
			context.device = new Device::IDEDevice(context.idePort);

		context.partition = new FS::Partition(context.device, entry.firstLBA * bs,
				(entry.lastLBA - entry.firstLBA + 1) * bs);
		tprintf("Selected partition \"%s\".\n", std::string(entry).c_str());
	}

	void list(const std::vector<std::string> &pieces, InputContext &context) {
		auto usage = [] { tprintf("Usage:\n- list controller\n- list partition\n- list port\n"); };
		if (pieces.size() < 2) {
			usage();
		} else if (pieces[1] == "ahci" || pieces[1] == "controllers" || pieces[1] == "controller" || pieces[1] == "cont") {
			listAHCI(context);
		} else if (pieces[1] == "gpt" || pieces[1] == "partition" || pieces[1] == "partitions" || pieces[1] == "part") {
			listGPT(context);
		} else if (pieces[1] == "port" || pieces[1] == "ports") {
			listPorts(context);
		} else {
			usage();
		}
	}

	void listPorts(InputContext &context) {
		if (!context.controller) {
			tprintf("No controller is selected.\n");
			return;
		}

		bool any_found = false;
		for (int i = 0; i < 32; ++i) {
			if (context.controller->ports[i]) {
				any_found = true;
				tprintf("[%d] %s: \"", i, AHCI::deviceTypes[static_cast<int>(context.controller->ports[i]->type)]);
				ATA::DeviceInfo info;
				context.controller->ports[i]->identify(info);
				char model[41];
				info.copyModel(model);
				tprintf("%s\"\n", model);
			}
		}

		if (!any_found)
			tprintf("No valid ports found.\n");
	}

	void listGPT(InputContext &context) {
		if ((context.ahci && !context.port) || (!context.ahci && context.idePort == -1)) {
			tprintf("No port is selected.\n");
			return;
		}

		MBR mbr;

		if (context.ahci)
			context.port->read(0, 512, &mbr);
		else
			IDE::readBytes(context.idePort, 512, 0, &mbr);

		if (!mbr.indicatesGPT()) {
			tprintf("MBR doesn't indicate the presence of a GPT.\n");
			return;
		}

		const size_t bs = context.ahci? AHCI::Port::BLOCKSIZE : IDE::SECTOR_SIZE;

		GPT::Header gpt;

		if (context.ahci)
			context.port->readBytes(sizeof(GPT::Header), bs, &gpt);
		else
			IDE::readBytes(context.idePort, sizeof(GPT::Header), bs, &gpt);

		tprintf("Signature:   0x%lx\n", gpt.signature);
		tprintf("Revision:    %d\n",    gpt.revision);
		tprintf("Header size: %d\n",    gpt.headerSize);
		tprintf("Current LBA: %ld\n",   gpt.currentLBA);
		tprintf("Other LBA:   %ld\n",   gpt.otherLBA);
		tprintf("First LBA:   %ld\n",   gpt.firstLBA);
		tprintf("Last LBA:    %ld\n",   gpt.lastLBA);
		tprintf("Start LBA:   %ld\n",   gpt.startLBA);
		tprintf("Partitions:  %d\n",    gpt.partitionCount);
		tprintf("Entry size:  %d\n",    gpt.partitionEntrySize);
		size_t offset = bs * gpt.startLBA;
		tprintf("GUID:        %s\n", std::string(gpt.guid).c_str());
		if (gpt.partitionEntrySize != sizeof(GPT::PartitionEntry)) {
			tprintf("Unsupported partition entry size.\n");
			return;
		}

		for (unsigned i = 0; i < gpt.partitionCount; ++i) {
			GPT::PartitionEntry entry;
			if (context.ahci)
				context.port->readBytes(gpt.partitionEntrySize, offset, &entry);
			else
				IDE::readBytes(context.idePort, gpt.partitionEntrySize, offset, &entry);
			if (entry.typeGUID) {
				tprintf("Partition %d: \"", i);
				entry.printName(false);
				tprintf("\"\n  Type GUID: %s\n  Partition GUID: %s\n  First = %ld, last = %ld\n",
					std::string(entry.typeGUID).c_str(), std::string(entry.partitionGUID).c_str(), entry.firstLBA,
					entry.lastLBA);
			}
			offset += gpt.partitionEntrySize;
		}
	}

	void listAHCI(InputContext &) {
		if (AHCI::controllers.empty()) {
			tprintf("No AHCI controllers found.\n");
		} else {
			tprintf("AHCI controllers:\n", AHCI::controllers.size());
			size_t i = 0;
			for (AHCI::Controller &controller: AHCI::controllers) {
				const auto &bdf = controller.device->bdf;
				tprintf("[%d] %x:%x:%x: ports =", i, bdf.bus, bdf.device, bdf.function);
				for (int i = 0; i < 32; ++i)
					if (controller.ports[i])
						tprintf(" %d", i);
				tprintf("\n");
				++i;
			}
			tprintf("Done.\n");
		}
	}

	void make(const std::vector<std::string> &, InputContext &context) {
		if (!context.driver) {
			tprintf("Driver isn't ready. Try init driver.\n");
		} else {
			tprintf("Creating partition...\n");
			context.driver->make(sizeof(FS::ThornFAT::DirEntry) * 5);
			tprintf("Initialized ThornFAT partition.\n");
		}
	}
}
