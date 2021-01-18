#include "hardware/SATA.h"
#include "memory/memset.h"

namespace DsOS::SATA {
	bool issueCommand(volatile AHCI::HBAPort &port, ATA::Command command, bool write, void *buffer, uint16_t prdtl,
	                  uint32_t byte_count, uint64_t start, uint16_t count) {
		AHCI::HBACommandHeader *header = (AHCI::HBACommandHeader *) &port.clb;
		const int slot = port.getCommandSlot();
		if (slot == -1)
			return false;

		header += slot;
		header->cfl = sizeof(AHCI::FISRegH2D) / 4;
		header->prefetchable = header->clearBusy = true;
		header->atapi = false;
		header->write = write;
		header->prdtl = prdtl;

		AHCI::HBACommandTable *table = (AHCI::HBACommandTable *) (uintptr_t) header->ctba;

		for (uint16_t i = 0; i < prdtl - 1; ++i) {
			table->prdtEntry[i].dba = (uintptr_t) buffer & 0xffffffff;
			table->prdtEntry[i].dbaUpper = (uintptr_t) buffer >> 32;
			table->prdtEntry[i].dbc = (8 << 10) - 1;
			table->prdtEntry[i].interrupt = false;
			buffer = (void *) ((uintptr_t) buffer + (8 << 10));
			byte_count -= 8 << 10;
		}

		table->prdtEntry[prdtl - 1].dba = (uintptr_t) buffer & 0xffffffff;
		table->prdtEntry[prdtl - 1].dbaUpper = (uintptr_t) buffer >> 32;
		table->prdtEntry[prdtl - 1].dbc = byte_count - 1;
		table->prdtEntry[prdtl - 1].interrupt = false;

		AHCI::FISRegH2D *fis = (AHCI::FISRegH2D *) table->cfis;

		memset(fis, 0, sizeof(AHCI::FISRegH2D));

		fis->type = AHCI::FISType::RegH2D;
		fis->c = 1;
		fis->command = command;
		fis->device = 0x40;

		fis->lba0 = (uint8_t) start;
		fis->lba1 = (uint8_t) (start >> 8);
		fis->lba2 = (uint8_t) (start >> 16);
		fis->lba3 = (uint8_t) (start >> 24);
		fis->lba4 = (uint8_t) (start >> 32);
		fis->lba5 = (uint8_t) (start >> 40);

		fis->countLow  = (uint8_t) (count & 0xff);
		fis->countHigh = (uint8_t) (count >> 8);

		port.ci = port.ci | (1 << slot);
		for (;;) {
			if ((port.ci & (1 << slot)) == 0)
				return !(port.is & (1 << 30));

			if (port.is & (1 << 30))
				return false;
		}

		return false;
	}
}
