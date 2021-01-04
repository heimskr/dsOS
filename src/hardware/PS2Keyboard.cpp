#include <stdint.h>

#include "arch/x86_64/PIC.h"
#include "hardware/Ports.h"
#include "hardware/PS2Keyboard.h"
#include "lib/printf.h"

namespace DsOS::PS2Keyboard {
	void onIRQ1() {
		x86_64::PIC::sendEOI(1);
		uint8_t scancode = DsOS::Ports::inb(0x60);
		printf("IRQ1: 0x%x\n", scancode);
	}
}
