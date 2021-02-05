#pragma once

#include <list>

#include "hardware/PCI.h"

namespace Thorn::UHCI {
	constexpr uint16_t COMMAND = 0;
	constexpr uint16_t STATUS = 2;
	constexpr uint16_t INTERRUPTS = 4;
	constexpr uint16_t FRAME_NUMBER = 6;
	constexpr uint16_t FRAME_BASE = 8;
	constexpr uint16_t START_OF_FRAME = 0xc;
	constexpr uint16_t PORT_STATUS = 0x10;

	struct Controller {
		PCI::Device *device;
		uint32_t address;
		uintptr_t bar4;

		Controller(PCI::Device *);
		void init();
		void reset();
		void enableInterrupts();
		uint16_t portStatus(int port);
		int countPorts();
	};

	void init();

	extern std::list<Controller> *controllers;
}
