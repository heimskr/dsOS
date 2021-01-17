#pragma once

#include <stddef.h>
#include <stdint.h>

#include "mmu.h"

namespace x86_64 {
	class PageTableWrapper {
		public:
			enum class Type {PML4, PDP, PD, PT};
			enum class PTDisplay {Full, Condensed, Hidden};

			uint64_t *entries;
			Type type;

			PageTableWrapper(uint64_t *, Type);

			void clear();

			void print(bool putc = true, bool show_pdpt = true, bool show_pdt = true, PTDisplay pt = PTDisplay::Condensed);

			uint64_t getPML4E(uint16_t pml4_index) const;
			uint64_t getPDPE(uint16_t pml4_index, uint16_t pdpt_index) const;
			uint64_t getPDE(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index) const;
			uint64_t getPTE(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) const;

			uint16_t getPML4Meta(uint16_t pml4_index) const;
			uint16_t getPDPTMeta(uint16_t pml4_index, uint16_t pdpt_index) const;
			uint16_t getPDTMeta(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index) const;
			uint16_t getPTMeta(uint16_t pml4_index, uint16_t pdpt_index, uint16_t pdt_index, uint16_t pt_index) const;

			static inline uint16_t getPML4Index(uint64_t       addr) { return (addr >> 39) & 0x1ff;          }
			static inline uint16_t getPML4Index(volatile void *addr) { return getPML4Index((uint64_t) addr); }

			static inline uint16_t getPDPTIndex(uint64_t       addr) { return (addr >> 30) & 0x1ff;          }
			static inline uint16_t getPDPTIndex(volatile void *addr) { return getPDPTIndex((uint64_t) addr); }

			static inline uint16_t getPDTIndex(uint64_t       addr) { return (addr >> 21) & 0x1ff;         }
			static inline uint16_t getPDTIndex(volatile void *addr) { return getPDTIndex((uint64_t) addr); }

			static inline uint16_t getPTIndex(uint64_t       addr) { return (addr >> 12) & 0x1ff;        }
			static inline uint16_t getPTIndex(volatile void *addr) { return getPTIndex((uint64_t) addr); }

			static inline uint16_t getOffset(uint64_t       addr) { return addr & 0xfff;               }
			static inline uint16_t getOffset(volatile void *addr) { return getOffset((uint64_t) addr); }

		private:
			void printMeta(uint64_t);
			void printPDPT(size_t i_shift, uint64_t pml4e, bool show_pdt, PTDisplay);
			void printPDT(size_t j_shift, uint64_t pdpe, PTDisplay);
			void printPT(size_t k_shift, uint64_t pde);
			void printCondensed(size_t k_shift, uint64_t pde);
	};
}
