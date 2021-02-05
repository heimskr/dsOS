// Based on code from RTEMS (https://www.rtems.org/), licensed under the GPL.

#include "arch/x86_64/APIC.h"
#include "arch/x86_64/CPU.h"
#include "arch/x86_64/PIC.h"
#include "hardware/Ports.h"
#include "lib/printf.h"
#include "Kernel.h"

volatile uint32_t *apic_base;

static bool timer_calibrated = false;

namespace x86_64::APIC {
	uint32_t lastTPS = 0;

	void init(Thorn::Kernel &kernel) {
		timer_calibrated = false;
		lastTPS = 0;
		printf("Initializing APIC.\n");
		uint64_t msr = rdmsr(MSR);
		apic_base = (uint32_t *) (msr & 0xffffff000);
		wrmsr(MSR, msr | ENABLE, msr >> 32);
		printf("APIC base: 0x%lx, ID: 0x%lx\n", apic_base, apic_base + REGISTER_APICID);
		kernel.pager.identityMap((void *) apic_base);
		apic_base[REGISTER_SPURIOUS] = SPURIOUS_ENABLE | BSP_VECTOR_SPURIOUS;
		PIC::remap(PIC::PIC1_REMAP_DEST, PIC::PIC2_REMAP_DEST);
		PIC::disable();
	}

	void initTimer(uint32_t frequency) {
		uint32_t ticks_per_second = lastTPS;
		if (!timer_calibrated) {
			uint32_t tick_collections[TIMER_NUM_CALIBRATIONS] = {0};
			uint64_t tick_total = 0;
			for (uint32_t i = 0; i < TIMER_NUM_CALIBRATIONS; i++)
				tick_total += tick_collections[i] = calibrateTimer();
			ticks_per_second = tick_total / TIMER_NUM_CALIBRATIONS;
			timer_calibrated = true;
			lastTPS = ticks_per_second;
		}
		uint32_t timer_reload_value = ticks_per_second / frequency;

		apic_base[REGISTER_LVT_TIMER] = BSP_VECTOR_APIC_TIMER | SELECT_TMR_PERIODIC;
		apic_base[REGISTER_TIMER_DIV] = TIMER_SELECT_DIVIDER;
		apic_base[REGISTER_TIMER_INITCNT] = timer_reload_value;
	}

	uint32_t calibrateTimer() {
		using namespace Thorn::Ports;
		apic_base[REGISTER_LVT_TIMER] = BSP_VECTOR_APIC_TIMER;
		apic_base[REGISTER_TIMER_DIV] = TIMER_SELECT_DIVIDER;

		// Enable the channel 2 timer gate and disable the speaker output.
		uint8_t chan2_value = (inb(PIT_PORT_CHAN2_GATE) | PIT_CHAN2_TIMER_BIT) & ~PIT_CHAN2_SPEAKER_BIT;
		outb(PIT_PORT_CHAN2_GATE, chan2_value);

		// Initialize PIT in one-shot mode on Channel 2.
		outb(PIT_PORT_MCR, PIT_SELECT_CHAN2 | PIT_SELECT_ACCESS_LOHI | PIT_SELECT_ONE_SHOT_MODE | PIT_SELECT_BINARY_MODE);

		/* Disable interrupts while we calibrate for 2 reasons:
		 *   - Writing values to the PIT should be atomic (for now, this is okay because we're the only ones ever
		 *     touching the PIT ports, but an interrupt resetting the PIT could mess calibration up).
		 *   - We need to make sure that no interrupts throw our synchronization of the APIC timer off. */
		disableInterrupts();

		/* Set PIT reload value */
		uint32_t pit_ticks = PIT_CALIBRATE_TICKS;
		uint8_t low_tick = pit_ticks & 0xff;
		uint8_t high_tick = (pit_ticks >> 8) & 0xff;

		outb(PIT_PORT_CHAN2, low_tick);
		outb(PIT_PORT_CHAN2, high_tick);

		// Start APIC timer's countdown.
		const uint32_t apic_calibrate_init_count = 0xffffffff;

		// Restart PIT by disabling the gated input and then re-enabling it.
		chan2_value &= ~PIT_CHAN2_TIMER_BIT;
		outb(PIT_PORT_CHAN2_GATE, chan2_value);
		chan2_value |= PIT_CHAN2_TIMER_BIT;
		outb(PIT_PORT_CHAN2_GATE, chan2_value);
		apic_base[REGISTER_TIMER_INITCNT] = apic_calibrate_init_count;

		while (pit_ticks <= PIT_CALIBRATE_TICKS) {
			// Send latch command to read multi-byte value atomically.
			outb(PIT_PORT_MCR, PIT_SELECT_CHAN2);
			pit_ticks = inb(PIT_PORT_CHAN2);
			pit_ticks |= inb(PIT_PORT_CHAN2) << 8;
		}

		uint32_t apic_currcnt = apic_base[REGISTER_TIMER_CURRCNT];

#ifdef DEBUG_APIC_TIMER
		printf("PIT stopped at 0x%lx\n", pit_ticks);
#endif

		// Stop APIC timer to calculate ticks to time ratio.
		apic_base[REGISTER_LVT_TIMER] = DISABLE;

		// Get counts passed since we started counting.
		uint32_t ticks_per_second = apic_calibrate_init_count - apic_currcnt;

#ifdef DEBUG_APIC_TIMER
		printf("APIC ticks passed in 1/%d of a second: 0x%lx\n", PIT_CALIBRATE_DIVIDER, ticks_per_second);
#endif

		// We ran the PIT for a fraction of a second.
		ticks_per_second = ticks_per_second * PIT_CALIBRATE_DIVIDER;

		// Re-enable interrupts now that calibration is complete.
		enableInterrupts();

		// Confirm that the APIC timer never hit 0 and IRQ'd during calibration.
		// assert(clock_driver_ticks == 0);
		// assert(ticks_per_second != 0 && ticks_per_second != apic_calibrate_init_count);

#ifdef DEBUG_APIC_TIMER
		printf("CPU frequency: 0x%llu\nAPIC ticks/sec: 0x%lx\n",
			// Multiply to undo effects of divider.
			(uint64_t) ticks_per_second * TIMER_DIVIDE_VALUE, ticks_per_second);
#endif

		return ticks_per_second;
	}

	void disableTimer() {
		apic_base[REGISTER_LVT_TIMER] = DISABLE;
		ticks = 0;
	}
}
