#include "hardware/UHCI.h"
#include "hardware/Ports.h"
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
		reset();

		device->writeInt(0x34, 0);
		device->writeInt(0x38, 0);

		// TODO: write IRQ

		// Enable bus mastering and IO access.
		device->writeWord(PCI::COMMAND, 5);
		// Disable legacy support for keyboards and mice.
		device->writeWord(0xc0, 0x8f00);

		enableInterrupts();
		Ports::outw(address + FRAME_NUMBER, 0);
		auto &pager = Kernel::getPager();
		void *physical = pager.allocateFreePhysicalAddress();
		if (0xffffffff < reinterpret_cast<uintptr_t>(physical)) {
			printf("[UHCI::Controller::init] Allocated page can't be represented with 32 bits!\n");
			return;
		}

		Ports::outl(address + FRAME_BASE, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(physical)));
		Ports::outw(address + START_OF_FRAME, 0x40); // Just in case!
		Ports::outw(address + STATUS, 0xffff);

		uint16_t command = Ports::inw(address + COMMAND);
		command &= ~(1 << 7);
		command |= 1;
		Ports::outw(address + COMMAND, command);
		resetPorts();
	}

	void Controller::reset() {
		using namespace Thorn::Ports;
		printf("Resetting UHCI controller.\n");

		for (int i = 0; i < 5; ++i) {
			outw(address + COMMAND, 0x0004);
			Kernel::wait(11, 1000);
			outw(address + COMMAND, 0x0004);
		}

		uint16_t command = inw(address + COMMAND);
		if (command != 0) {
			printf("Command (0x%x) isn't zero!\n", command);
		}

		const uint16_t status = inw(address + STATUS);
		if (status != 0x20) {
			printf("Status (0x%x) isn't 0x20!\n", status);
		}

		outw(address + STATUS, 0xff);
		const uint16_t sof = inw(address + START_OF_FRAME);
		if (sof != 0x40) {
			printf("SOF (0x%x) isn't 0x40!\n", sof);
		}

		outw(address + COMMAND, 2);
		Kernel::wait(42, 1000);
		command = inw(address + COMMAND);
		if (command & 2) {
			printf("Command (0x%x) & 2 is true!\n", command);
		}

		printf("Finished resetting UHCI controller.\n");
	}

	void Controller::resetPorts() {
		int count = countPorts();
		for (uint8_t port = 0; port < count; ++port) {
			int status = portStatus(port);
			status |= 1 << 9;
			setPortStatus(port, status);
			Kernel::wait(5, 100);
			status &= ~(1 << 9);
			setPortStatus(port, status);
			Kernel::wait(1, 100);
			printf("Port %d: low speed? %y\n", port, (portStatus(port) & (1 << 8)) != 0);
		}
	}

	void Controller::enableInterrupts() {
		Ports::outw(address + INTERRUPTS, 0xf);
	}

	uint16_t Controller::portStatus(uint8_t port) {
		return Ports::inw(address + PORT_STATUS + 2 * port);
	}

	void Controller::setPortStatus(uint8_t port, uint16_t status) {
		Ports::outw(address + PORT_STATUS + 2 * port, status);
	}

	bool Controller::portValid(uint8_t port) {
		Ports::port_t addr = address + PORT_STATUS + 2 * port;
		if ((Ports::inw(addr) & 0x80) == 0)
			return false;

		Ports::outw(addr, Ports::inw(addr) & ~0x80);
		if ((Ports::inw(addr) & 0x80) == 0)
			return false;

		Ports::outw(addr, Ports::inw(addr) | 0x80);
		if ((Ports::inw(addr) & 0x80) == 0)
			return false;

		Ports::outw(addr, Ports::inw(addr) | 0xa);
		if (Ports::inw(addr) & 0xa)
			return false;

		return true;
	}

	int Controller::countPorts() {
		uint8_t port;
		for (port = 0; portValid(port); ++port);
		return port;
	}

	void init() {
		controllers = new std::list<Controller>();
	}
}
