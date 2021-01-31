#include "hardware/SATA.h"
#include "memory/memset.h"

namespace DsOS::SATA {
	/*
	bool issueCommand(AHCI::Controller &controller, AHCI::Port &port, ATA::Command command, bool write, void *buffer,
	                  uint16_t prdtl, uint32_t byte_count, uint64_t start) {
		AHCI::HBACommandHeader *header = (AHCI::HBACommandHeader *) ((uintptr_t) port.clb | ((uintptr_t) port.clbu << 32));
		const uint64_t count = (count + 512 - 1) / 512;
		// printf("header: 0x%lx\n", header);
		const int slot = port.getCommandSlot(*controller.abar);
		if (slot == -1)
			return false;

		// printf("slot: %d\n", slot);

		header += slot;
		header->cfl = sizeof(AHCI::FISRegH2D) / sizeof(uint32_t);
		header->atapi = false;
		header->write = write;
		header->prdtl = prdtl;
		header->prefetchable = true;
		header->clearBusy = true;

		AHCI::HBACommandTable *table = (AHCI::HBACommandTable *) ((uintptr_t) header->ctba | ((uintptr_t) header->ctbau << 32));
		// printf("table: 0x%lx\n", table);

		uint16_t i;
		for (i = 0; i < prdtl - 1; ++i) {
			table->prdtEntry[i].dba = (uintptr_t) buffer & 0xffffffff;
			table->prdtEntry[i].dbaUpper = (uintptr_t) buffer >> 32;
			table->prdtEntry[i].dbc = (8 << 10) - 1;
			table->prdtEntry[i].interrupt = false;
			buffer = (void *) ((uintptr_t) buffer + (8 << 10));
			byte_count -= 8 << 10;
		}

		table->prdtEntry[i].dba = (uintptr_t) buffer & 0xffffffff;
		table->prdtEntry[i].dbaUpper = (uintptr_t) buffer >> 32;
		table->prdtEntry[i].dbc = byte_count - 1;
		table->prdtEntry[i].interrupt = false;

		AHCI::FISRegH2D *fis = (AHCI::FISRegH2D *) table->cfis;
		// printf("fis: 0x%lx\n", fis);

		memset(fis, 0, sizeof(AHCI::FISRegH2D));

		fis->type = AHCI::FISType::RegH2D;
		fis->c = 1;
		fis->command = command;

		fis->control = 8;

		fis->lba0 = (uint8_t) start;
		fis->lba1 = (uint8_t) (start >> 8);
		fis->lba2 = (uint8_t) (start >> 16);
		fis->device = 1 << 6;
		fis->lba3 = (uint8_t) (start >> 24);
		fis->lba4 = (uint8_t) (start >> 32);
		fis->lba5 = (uint8_t) (start >> 40);

		fis->countLow  = (uint8_t) (count & 0xff);
		fis->countHigh = (uint8_t) (count >> 8) & 0xff;

		printf("tfd = 0x%lx / %b\n", port.tfd, port.tfd);

		port.ci = port.ci | (1 << slot);
		port.sact = port.sact | (1 << slot); // ???
		// port.cmd = port.cmd | AHCI::HBA_PxCMD_ST; // ???
		for (;;) {
			if ((port.ci & (1 << slot)) == 0)
				return !(port.is & AHCI::HBA_PxIS_TFES);

			if (port.is & AHCI::HBA_PxIS_TFES)
				return false;
		}


		return false;
	}

	bool read(AHCI::Controller &controller, volatile AHCI::HBAPort &port, uint64_t start, uint32_t count,
	          void *buffer_) {
		uint64_t block_count = (count + 512 - 1) / 512;
        char *buffer = reinterpret_cast<char *>(buffer_);

		while (8 <= block_count && count) {
			uint32_t size = 512 * 8;
			if (count < size)
				size = count;
			if (!size)
				break;

		}
	}
	//*/
}
