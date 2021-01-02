#pragma once

#include <stdint.h>

#include "mmu.h"

namespace x86_64 {
	class PageTable {
		public:
			enum class Type {PML4, PDP, PD, PT};

			uint64_t *entries;
			Type type;

			PageTable(uint64_t *, Type);

			void clear();

			/** Returns true if an entry was assigned. */
			bool assign(uint16_t pml4, uint16_t pdpt, uint16_t pdt, uint16_t pt);

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

		private:
			void printMeta(uint64_t);
	};
}
