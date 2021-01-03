#pragma once

// Based on code from RTEMS (https://www.rtems.org/), licensed under the GPL.

#include <stdint.h>

namespace DsOS {
	class Kernel;
}

namespace x86_64::APIC {
	constexpr uint32_t APIC_ENABLE = 0x800;
	constexpr uint32_t APIC_MSR = 0x1b;
	constexpr uint32_t APIC_REGISTER_APICID = 0x20 >> 2;
	constexpr uint32_t APIC_REGISTER_SPURIOUS = 0xf0 >> 2;
	constexpr uint32_t APIC_REGISTER_TIMER_DIV = 0x3e0 >> 2;
	constexpr uint32_t APIC_SPURIOUS_ENABLE = 0x100;
	constexpr uint32_t BSP_VECTOR_SPURIOUS = 0xff;

	void init(DsOS::Kernel &);
}