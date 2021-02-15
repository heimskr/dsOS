#pragma once

// MIT License

// Copyright (c) 2018 finixbit

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <string>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>    // O_RDONLY
#include <sys/stat.h> // For the size of the file, fstat
#include <sys/mman.h> // mmap, MAP_PRIVATE
#include <vector>
#include <elf.h>      // Elf64_Shdr
#include <fcntl.h>

namespace Elf {
	struct Section {
		int sectionIndex = 0;
		std::intptr_t sectionOffset, sectionAddr;
		std::string sectionName;
		std::string sectionType;
		int sectionSize, sectionEntSize, sectionAddrAlign;
	};

	struct Segment {
		std::string segmentType, segmentFlags;
		long segmentOffset, segmentVirtaddr, segmentPhysaddr, segmentFilesize, segmentMemsize;
		int segmentAlign;
	};

	struct Symbol {
		std::string symbolIndex;
		std::intptr_t symbolValue;
		int symbolNum = 0, symbolSize = 0;
		std::string symbolType, symbolBind, symbolVisibility, symbolName, symbolSection;
	};

	struct Relocation {
		std::intptr_t relocationOffset, relocationInfo, relocationSymbolValue;
		std::string   relocationType, relocationSymbolName, relocationSectionName;
		std::intptr_t relocationPltAddress;
	};

	class ElfParser {
		public:
			ElfParser(const std::string &program_text) {
				loadProgramText(program_text);
			}

			std::vector<Section> getSections();
			std::vector<Segment> getSegments();
			std::vector<Symbol> getSymbols();
			std::vector<Relocation> getRelocations();

		private:
			std::string programText;
			void loadProgramText(const std::string &);

			const char * getSectionType(int tt);

			const char * getSegmentType(uint32_t seg_type);
			std::string getSegmentFlags(uint32_t seg_flags);

			const char * getSymbolType(uint8_t sym_type);
			const char * getSymbolBind(uint8_t sym_bind);
			const char * getSymbolVisibility(uint8_t sym_vis);
			std::string getSymbolIndex(uint16_t sym_idx);

			const char * getRelocationType(uint64_t rela_type);
			std::intptr_t getRelSymbolValue(uint64_t sym_idx, const std::vector<Symbol> &syms);
			std::string getRelSymbolName(uint64_t sym_idx, const std::vector<Symbol> &syms);

			std::string m_program_path;
			uint8_t *m_mmap_program;
	};
}
