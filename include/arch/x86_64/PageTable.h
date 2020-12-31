#pragma once

#include <stdint.h>

#include "mmu.h"

namespace x86_64 {
	struct PageTable {
		enum class Type {PML4, PDP, PD, PT};

		uint64_t *entries;
		Type type;

		PageTable(uint64_t *, Type);
		void clear();
		void print();

		uint64_t getPML4E(uint16_t pml4_index) const;
		uint64_t getPDPE(uint16_t pml4_index, uint16_t pdpt_index) const;
		uint64_t getPDE(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index) const;
		uint64_t getPTE(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) const;

		uint16_t getPML4Meta(uint16_t pml4_index) const;
		uint16_t getPDPTMeta(uint16_t pml4_index, uint16_t pdpt_index) const;
		uint16_t getPDTMeta(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index) const;
		uint16_t getPTMeta(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) const;

		static inline uint16_t getPML4Index(uint64_t addr) {
			return (addr >> 39) & 0x1ff;
		}

		static inline uint16_t getPDPTIndex(uint64_t addr) {
			return (addr >> 30) & 0x1ff;
		}

		static inline uint16_t getPDTIndex(uint64_t addr) {
			return (addr >> 21) & 0x1ff;
		}

		static inline uint16_t getPTIndex(uint64_t addr) {
			return (addr >> 12) & 0x1ff;
		}

		static inline uint16_t getOffset(uint16_t addr) {
			return addr & 0xfff;
		}
	};
}
