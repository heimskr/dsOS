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

	class Controller {
		private:
			void reset();
			void resetPorts();

		public:
			PCI::Device *device;
			uint32_t address;
			uintptr_t bar4;

			Controller(PCI::Device *);
			void init();
			void enableInterrupts();
			uint16_t portStatus(int port);
			void setPortStatus(int port, uint16_t status);
			int countPorts();
	};

	void init();

	extern std::list<Controller> *controllers;
}
