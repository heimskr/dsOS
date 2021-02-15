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

#include "lib/ElfParser.h"
#include "lib/printf.h"
#include "Kernel.h"

namespace Elf {
	std::vector<Section> ElfParser::getSections() {
		Elf64_Ehdr *ehdr = (Elf64_Ehdr *) programText.c_str();
		Elf64_Shdr *shdr = (Elf64_Shdr *) (programText.c_str() + ehdr->e_shoff);
		int shnum = ehdr->e_shnum;

		Elf64_Shdr *sh_strtab = &shdr[ehdr->e_shstrndx];
		const char * const sh_strtab_p = (const char *) programText.c_str() + sh_strtab->sh_offset;

		std::vector<Section> sections;
		for (int i = 0; i < shnum; ++i) {
			Section section;
			section.index     = i;
			section.name      = std::string(sh_strtab_p + shdr[i].sh_name);
			section.type      = getSectionType(shdr[i].sh_type);
			section.addr      = shdr[i].sh_addr;
			section.offset    = shdr[i].sh_offset;
			section.size      = shdr[i].sh_size;
			section.entSize   = shdr[i].sh_entsize;
			section.addrAlign = shdr[i].sh_addralign;
			sections.push_back(section);
			printf("Size: %lu -> %lu\n", sections.size(), sections.size() * sizeof(Section));
		}

		return sections;
	}

	std::vector<Segment> ElfParser::getSegments() {
		Elf64_Ehdr *ehdr = (Elf64_Ehdr *) programText.c_str();
		Elf64_Phdr *phdr = (Elf64_Phdr *) (programText.c_str() + ehdr->e_phoff);
		const int phnum = ehdr->e_phnum;

		std::vector<Segment> segments;
		for (int i = 0; i < phnum; ++i) {
			Segment segment;
			segment.type     = getSegmentType(phdr[i].p_type);
			segment.offset   = phdr[i].p_offset;
			segment.virtaddr = phdr[i].p_vaddr;
			segment.physaddr = phdr[i].p_paddr;
			segment.filesize = phdr[i].p_filesz;
			segment.memsize  = phdr[i].p_memsz;
			segment.flags    = getSegmentFlags(phdr[i].p_flags);
			segment.align    = phdr[i].p_align;
			
			segments.push_back(segment);
		}
		return segments;
	}

	std::vector<Symbol> ElfParser::getSymbols() {
		std::vector<Section> secs = getSections();

		// get strtab
		const char *sh_strtab_p = nullptr;
		for (const Section &sec: secs) {
			if ((sec.type == "SHT_STRTAB") && (sec.name == ".strtab")) {
				sh_strtab_p = (const char *) programText.c_str() + sec.offset;
				break;
			}
		}

		// get dynstr
		const char *sh_dynstr_p = nullptr;
		for (const Section &sec: secs) {
			if ((sec.type == "SHT_STRTAB") && (sec.name == ".dynstr")) {
				sh_dynstr_p = (const char *) programText.c_str() + sec.offset;
				break;
			}
		}

		std::vector<Symbol> symbols;
		for (const Section &sec: secs) {
			if ((sec.type != "SHT_SYMTAB") && (sec.type != "SHT_DYNSYM"))
				continue;

			int total_syms = sec.size / sizeof(Elf64_Sym);
			Elf64_Sym *syms_data = (Elf64_Sym*) (programText.c_str() + sec.offset);

			for (int i = 0; i < total_syms; ++i) {
				Symbol symbol;
				symbol.num        = i;
				symbol.value      = syms_data[i].st_value;
				symbol.size       = syms_data[i].st_size;
				symbol.type       = getSymbolType(syms_data[i].st_info);
				symbol.bind       = getSymbolBind(syms_data[i].st_info);
				symbol.visibility = getSymbolVisibility(syms_data[i].st_other);
				symbol.index      = getSymbolIndex(syms_data[i].st_shndx);
				symbol.section    = sec.name;
				
				if (sec.type == "SHT_SYMTAB")
					symbol.name = std::string(sh_strtab_p + syms_data[i].st_name);
				
				if (sec.type == "SHT_DYNSYM")
					symbol.name = std::string(sh_dynstr_p + syms_data[i].st_name);
				
				symbols.push_back(symbol);
			}
		}
		return symbols;
	}

	std::vector<Relocation> ElfParser::getRelocations() {
		std::vector<Section> secs = getSections();
		std::vector<Symbol> syms = getSymbols();
		
		int  plt_entry_size = 0;
		long plt_vma_address = 0;

		for (const Section &sec: secs) {
			if (sec.name == ".plt") {
				plt_entry_size  = sec.entSize;
				plt_vma_address = sec.addr;
				break;
			}
		}

		std::vector<Relocation> relocations;
		for (const Section &sec: secs) {

			if (sec.type != "SHT_RELA")
				continue;

			int total_relas = sec.size / sizeof(Elf64_Rela);
			Elf64_Rela *relas_data  = (Elf64_Rela *) (programText.c_str() + sec.offset);

			for (int i = 0; i < total_relas; ++i) {
				Relocation rel;
				rel.offset = static_cast<std::intptr_t>(relas_data[i].r_offset);
				rel.info   = static_cast<std::intptr_t>(relas_data[i].r_info);
				rel.type   = getRelocationType(relas_data[i].r_info);
				rel.symbolValue = getRelSymbolValue(relas_data[i].r_info, syms);
				rel.symbolName  = getRelSymbolName(relas_data[i].r_info, syms);
				rel.pltAddress  = plt_vma_address + (i + 1) * plt_entry_size;
				rel.sectionName = sec.name;
				relocations.push_back(rel);
			}
		}
		return relocations;
	}

	void ElfParser::loadProgramText(const std::string &text) {
		Elf64_Ehdr *header = (Elf64_Ehdr *) text.c_str();
		if (header->e_ident[EI_CLASS] != ELFCLASS64) {
			printf("Only 64-bit files supported\n");
			Thorn::Kernel::perish();
		}

		programText = text;
	}

	const char * ElfParser::getSectionType(int tt) {
		if (tt < 0)
			return "UNKNOWN";

		switch (tt) {
			case 0: return "SHT_NULL";      /* Section header table entry unused */
			case 1: return "SHT_PROGBITS";  /* Program data */
			case 2: return "SHT_SYMTAB";    /* Symbol table */
			case 3: return "SHT_STRTAB";    /* String table */
			case 4: return "SHT_RELA";      /* Relocation entries with addends */
			case 5: return "SHT_HASH";      /* Symbol hash table */
			case 6: return "SHT_DYNAMIC";   /* Dynamic linking information */
			case 7: return "SHT_NOTE";      /* Notes */
			case 8: return "SHT_NOBITS";    /* Program space with no data (bss) */
			case 9: return "SHT_REL";       /* Relocation entries, no addends */
			case 11: return "SHT_DYNSYM";   /* Dynamic linker symbol table */
			default: return "UNKNOWN";
		}
		return "UNKNOWN";
	}

	const char * ElfParser::getSegmentType(uint32_t seg_type) {
		switch (seg_type) {
			case PT_NULL:         return "NULL";         // Program header table entry unused 
			case PT_LOAD:         return "LOAD";         // Loadable program segment
			case PT_DYNAMIC:      return "DYNAMIC";      // Dynamic linking information
			case PT_INTERP:       return "INTERP";       // Program interpreter
			case PT_NOTE:         return "NOTE";         // Auxiliary information
			case PT_SHLIB:        return "SHLIB";        // Reserved
			case PT_PHDR:         return "PHDR";         // Entry for header table itself
			case PT_TLS:          return "TLS";          // Thread-local storage segment
			case PT_NUM:          return "NUM";          // Number of defined types
			case PT_LOOS:         return "LOOS";         // Start of OS-specific
			case PT_GNU_EH_FRAME: return "GNU_EH_FRAME"; // GCC .eh_frame_hdr segment
			case PT_GNU_STACK:    return "GNU_STACK";    // Indicates stack executability
			case PT_GNU_RELRO:    return "GNU_RELRO";    // Read-only after relocation
			// case PT_LOSUNW:       return "LOSUNW";
			case PT_SUNWBSS:      return "SUNWBSS";      // Sun Specific segment
			case PT_SUNWSTACK:    return "SUNWSTACK";    // Stack segment
			// case PT_HISUNW:       return "HISUNW";
			case PT_HIOS:         return "HIOS";         // End of OS-specific
			case PT_LOPROC:       return "LOPROC";       // Start of processor-specific
			case PT_HIPROC:       return "HIPROC";       // End of processor-specific
			default:              return "UNKNOWN";
		}
	}

	std::string ElfParser::getSegmentFlags(uint32_t seg_flags) {
		std::string flags;

		if (seg_flags & PF_R)
			flags.append("R");

		if (seg_flags & PF_W)
			flags.append("W");

		if (seg_flags & PF_X)
			flags.append("E");

		return flags;
	}

	const char * ElfParser::getSymbolType(uint8_t sym_type) {
		switch (ELF32_ST_TYPE(sym_type)) {
			case 0:  return "NOTYPE";
			case 1:  return "OBJECT";
			case 2:  return "FUNC";
			case 3:  return "SECTION";
			case 4:  return "FILE";
			case 6:  return "TLS";
			case 7:  return "NUM";
			case 10: return "LOOS";
			case 12: return "HIOS";
			default: return "UNKNOWN";
		}
	}

	const char * ElfParser::getSymbolBind(uint8_t sym_bind) {
		switch (ELF32_ST_BIND(sym_bind)) {
			case 0:  return "LOCAL";
			case 1:  return "GLOBAL";
			case 2:  return "WEAK";
			case 3:  return "NUM";
			case 10: return "UNIQUE";
			case 12: return "HIOS";
			case 13: return "LOPROC";
			default: return "UNKNOWN";
		}
	}

	const char * ElfParser::getSymbolVisibility(uint8_t sym_vis) {
		switch (ELF32_ST_VISIBILITY(sym_vis)) {
			case 0:  return "DEFAULT";
			case 1:  return "INTERNAL";
			case 2:  return "HIDDEN";
			case 3:  return "PROTECTED";
			default: return "UNKNOWN";
		}
	}

	std::string ElfParser::getSymbolIndex(uint16_t sym_idx) {
		switch (sym_idx) {
			case SHN_ABS:    return "ABS";
			case SHN_COMMON: return "COM";
			case SHN_UNDEF:  return "UND";
			case SHN_XINDEX: return "COM";
			default: return std::to_string(sym_idx);
		}
	}

	const char * ElfParser::getRelocationType(uint64_t rela_type) {
		switch (ELF64_R_TYPE(rela_type)) {
			case 1:  return "R_X86_64_32";
			case 2:  return "R_X86_64_PC32";
			case 5:  return "R_X86_64_COPY";
			case 6:  return "R_X86_64_GLOB_DAT";
			case 7:  return "R_X86_64_JUMP_SLOT";
			default: return "OTHERS";
		}
	}

	std::intptr_t ElfParser::getRelSymbolValue(uint64_t sym_idx, const std::vector<Symbol> &syms) {
		for (const Symbol &sym: syms)
			if (sym.num == static_cast<int>(ELF64_R_SYM(sym_idx)))
				return sym.value;
		return 0;
	}

	std::string ElfParser::getRelSymbolName(uint64_t sym_idx, const std::vector<Symbol> &syms) {
		for (const Symbol &sym: syms)
			if (sym.num == static_cast<int>(ELF64_R_SYM(sym_idx)))
				return sym.name;
		return "";
	}
}
