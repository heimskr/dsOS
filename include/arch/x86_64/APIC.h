#pragma once

#include <stdint.h>

namespace x86_64::APIC {
	constexpr uint32_t APIC_MSR = 0x1b;
	constexpr uint32_t APIC_ENABLE = 0x800;
	constexpr uint32_t APIC_REGISTER_APICID = 0x20 >> 2;
	constexpr uint32_t APIC_REGISTER_TIMER_DIV = 0x3e0 >> 2;

	void init();
}