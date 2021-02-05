#include "hardware/UHCI.h"
#include "Kernel.h"

namespace Thorn::UHCI {
	std::list<Controller> *controllers = nullptr;

	Controller::Controller(PCI::Device *device_): device(device_) {
		const uint32_t bar4 = device_->rawBAR(4);
		address = bar4;
		if (address & 1) {
			address &= ~0b11111; // Or maybe just ~1?
		} else {
			printf("[UHCI::Controller] bar4 (0x%lx) indicates MMIO!\n", bar4);
			Kernel::perish();
		}

		uint8_t x60 = device_->readByte(0x60);
		if (x60 != 0x10) {
			printf("[UHCI::Controller] Byte at offset 0x60 is %x; expected 0x10!\n", x60);
			Kernel::perish();
		}
	}

	void Controller::init() {
		device->writeInt(0x34, 0);
		device->writeInt(0x38, 0);

		// TODO: write IRQ

		// Enable bus mastering and IO access.
		device->writeWord(PCI::COMMAND, 5);
		// Disable legacy support for keyboards and mice.
		device->writeWord(0xc0, 0x8f00);
	}

	void init() {
		controllers = new std::list<Controller>();
	}
}
