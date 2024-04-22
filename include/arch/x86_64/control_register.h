#ifndef ARCH_X86_64_CONTROL_REGISTER_H_
#define ARCH_X86_64_CONTROL_REGISTER_H_

#define CONTROL_REGISTER0_PROTECTED_MODE_ENABLED (1 << 0)
#define CONTROL_REGISTER0_MONITOR_COPROCESSOR (1 << 1)
#define CONTROL_REGISTER0_EXTENSION_TYPE (1 << 4)
#define CONTROL_REGISTER0_PAGE (1 << 31)

#define CONTROL_REGISTER4_PAGE_SIZE_EXTENSION (1 << 4)
#define CONTROL_REGISTER4_PHYSICAL_ADDRESS_EXTENSION (1 << 5)
#define CONTROL_REGISTER4_OSFXSR (1 << 9)
#define CONTROL_REGISTER4_OSXMMEXCPT (1 << 10)

#ifndef __ASSEMBLER__
#include <stdint.h>
namespace x86_64 {
	uint64_t getCR0();
	/** This will cause a #UD exception. */
	uint64_t getCR1();
	uint64_t getCR2();
	uint64_t getCR3();
	uint64_t getCR4();
}
#endif

#endif
