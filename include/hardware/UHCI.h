#pragma once

#include <list>

#include "hardware/PCI.h"

namespace Thorn::UHCI {
	struct Controller {
		PCI::Device *device;
		uint32_t address;
		uintptr_t bar4;

		Controller(PCI::Device *);
		void init();
	};

	void init();

	extern std::list<Controller> *controllers;
}
