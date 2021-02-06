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
			uint16_t portStatus(uint8_t port);
			void setPortStatus(uint8_t port, uint16_t status);
			bool portValid(uint8_t port);
			int countPorts();
	};

	struct TransferDescriptor {
		bool terminate: 1;
		bool queue: 1;
		uint8_t rsv0: 2;
		uint32_t linkPointer: 28;
		uint16_t actualLength: 11;
		uint8_t rsv1: 5;
		uint8_t status: 8;
		bool interruptOnComplete: 1;
		bool isochronousSelect: 1;
		bool lowSpeed: 1;
		uint8_t errorCount: 2;
		bool shortPacketDetect: 1;
		uint8_t rsv2: 2;
		uint8_t packetIdentification: 8; // IN (0x69), OUT (0xe1) or SETUP (0x2d)
		uint8_t deviceAddress: 7;
		uint8_t endPointer: 4;
		bool dataToggle: 1;
		uint8_t rsv3: 1;
		uint16_t maxLength: 11;
		uint32_t bufferPointer;
		uint32_t rsv4;
		uint32_t rsv5;
		uint32_t rsv6;
		uint32_t rsv7;
	} __attribute__((packed));

	void init();

	extern std::list<Controller> *controllers;
}
