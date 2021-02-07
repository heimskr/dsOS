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

namespace Thorn {
	void runTests() {
		// testUHCI();
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

	void testAHCI() {
		AHCI::Controller *last_controller = nullptr;

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

					AHCI::controllers->push_back(AHCI::Controller(PCI::initDevice({bus, device, function})));
					AHCI::Controller &controller = AHCI::controllers->back();
					if (!Kernel::instance) {
						printf("[testAHCI] Kernel instance is null!\n");
						return;
					}
					controller.init(*Kernel::instance);
					last_controller = &controller;
					volatile AHCI::HBAMemory *abar = controller.abar;
					printf("Controller at %x:%x:%x (abar = 0x%llx):", bus, device, function, abar);
					printf(" %dL.%dP ", controller.device->getInterruptLine(), controller.device->getInterruptPin());
					printf(" cap=%b", abar->cap);
					printf("\n");
					// wait(1);

					for (int i = 0; i < 32; ++i) {
						volatile AHCI::HBAPort &port = abar->ports[i];
						if (port.clb == 0)
							continue;

						controller.ports[i] = new AHCI::Port(&port, abar);
						controller.ports[i]->init();
						ATA::DeviceInfo info;
						controller.ports[i]->identify(info);
						// wait(5);

						char model[41];
						info.copyModel(model);
						printf("Model: \"%s\"\n", model);
						printf("%d done.\n", i);
					}
					printf("\nNext.\n");
				}
		printf("Done.\n");

		if (last_controller) {
			MBR mbr;
			AHCI::Port *port = nullptr;
			for (int i = 0; i < 32; ++i)
				if (last_controller->ports[i]) {
					port = last_controller->ports[i];
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
		} else printf(":(\n");
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
							Terminal::left();
							if (0 < index)
								--index;
							break;
						case Keyboard::InputKey::KeyRightArrow:
							Terminal::right();
							if (index < text.size())
								++index;
							break;
						case Keyboard::InputKey::KeyBackspace:
							Terminal::left();
							printf(" ");
							Terminal::left();
							if (0 < index)
								text.erase(--index);
							break;
						case Keyboard::InputKey::KeyEnter:
							// printf("\n");
							Terminal::clear();
							printf("%s", text.c_str());
							Terminal::row = text.size() / Terminal::VGA_WIDTH;
							Terminal::column = text.size() % Terminal::VGA_WIDTH;
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
			// asm volatile("sti");
			// asm volatile("hlt");
		}
	}
}