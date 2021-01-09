// http://wiki.osdev.org/IDE

#include "hardware/IDE.h"
#include "hardware/Ports.h"
#include "lib/printf.h"

using DsOS::Ports::inb;
using DsOS::Ports::outb;

bool irqInvoked = false;

namespace DsOS::IDE {
	Device devices[4];
	ChannelRegisters channels[2];
	uint8_t ideBuffer[2048] = {0};
	static uint8_t ideStatus = 0;
	static uint8_t atapiPacket[12] = {0xa8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

	static inline void insl(int port, void *addr, int cnt) {
		asm volatile("cld; repne; insl" : "=D"(addr), "=c"(cnt) : "d"(port), "0"(addr), "1"(cnt) : "memory", "cc");
	}

	void init() {
		init(0x1f0, 0x3f6, 0x170, 0x376, 0);
	}

	int readSectors(uint8_t drive, uint8_t numsects, uint32_t lba, char *buffer) {
		int status = -1;
		if (drive > 3 || devices[drive].reserved == 0) {
			status = 0x1; // Drive not found!
		} else if (((lba + numsects) > devices[drive].size) && (devices[drive].type == IDE_ATA)) {
			status = 0x2; // Seeking to invalid position.
		} else {
			uint8_t err = 0;
			if (devices[drive].type == IDE_ATA)
				err = accessATA(ATA_READ, drive, lba, numsects, buffer);
			else if (devices[drive].type == IDE_ATAPI)
				for (uint8_t i = 0; i < numsects; i++) {
					printf("ok.\n");
					err = readATAPI(drive, lba + i, buffer + (i * 2048));
				}
			status = printError(drive, err);
		}
		return -status;
	}

	int writeSectors(uint8_t drive, uint8_t numsects, uint32_t lba, const char *buffer) {
		if (drive > 3 || devices[drive].reserved == 0) {
			ideStatus = 0x1; // Drive not found!
		} else if (((lba + numsects) > devices[drive].size) && (devices[drive].type == IDE_ATA)) {
			ideStatus = 0x2; // Seeking to invalid position.
		} else {
			uint8_t err = 0;
			if (devices[drive].type == IDE_ATA)
				err = accessATA(ATA_WRITE, drive, lba, numsects, const_cast<char *>(buffer));
			else if (devices[drive].type == IDE_ATAPI)
				err = 4; // Write-protected
			ideStatus = printError(drive, err);
		}

		return -ideStatus;
	}

	static void __delay(size_t ms) {
		for (size_t i = 0; i < 10 * ms; i++);
	}

	void init(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4) {
		int count = 0;

		// Detect I/O ports that interface the IDE controller
		channels[ATA_PRIMARY].base	  = (bar0 & 0xFFFFFFFC) + 0x1F0 * (!bar0);
		channels[ATA_PRIMARY].ctrl	  = (bar1 & 0xFFFFFFFC) + 0x3F6 * (!bar1);
		channels[ATA_SECONDARY].base  = (bar2 & 0xFFFFFFFC) + 0x170 * (!bar2);
		channels[ATA_SECONDARY].ctrl  = (bar3 & 0xFFFFFFFC) + 0x376 * (!bar3);
		channels[ATA_PRIMARY].bmide	  = (bar4 & 0xFFFFFFFC) + 0; // Bus Master IDE
		channels[ATA_SECONDARY].bmide = (bar4 & 0xFFFFFFFC) + 8; // Bus Master IDE
		// Disable IRQs
		write(ATA_PRIMARY, ATA_REG_CONTROL, 2);
		write(ATA_SECONDARY, ATA_REG_CONTROL, 2);
		// Detect ATA/ATAPI devices
		for (int i = 0; i < 2; i++)
			for (int j = 0; j < 2; j++) {

				uint8_t err = 0, type = IDE_ATA, status;
				devices[count].reserved = 0; // Assuming there's no drive here

				// Select drive
				write(i, ATA_REG_HDDEVSEL, 0xA0 | (j << 4));
				__delay(1); // Wait 1ms for drive select to work

				// Send ATA identify command
				write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
				__delay(1);

				if (read(i, ATA_REG_STATUS) == 0)
					continue; // If status = 0, no device.

				while (1) {
					status = read(i, ATA_REG_STATUS);
					if ((status & ATA_SR_ERR)) {
						err = 1;
						break;
					} // If err, device is not ATA.
					if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
						break; // Everything is fine
				}

				// Probe for ATAPI devices
				if (err != 0) {
					uint8_t cl = read(i, ATA_REG_LBA1);
					uint8_t ch = read(i, ATA_REG_LBA2);

					if (cl == 0x14 && ch == 0xEB)
						type = IDE_ATAPI;
					else if (cl == 0x69 && ch == 0x96)
						type = IDE_ATAPI;
					else
						continue; // Unknown type (may not be a device)

					write(i, ATA_REG_COMMAND, ATA_CMD_IDENTIFY_PACKET);
					__delay(1);
				}

				// Read identification space of the device
				readBuffer(i, ATA_REG_DATA, (void *) ideBuffer, 128);

				// Read device parameters
				devices[count].reserved	    = true;
				devices[count].type	        = type;
				devices[count].channel      = i;
				devices[count].drive        = j;
				devices[count].signature    = *((uint16_t *) (ideBuffer + ATA_IDENT_DEVICETYPE));
				devices[count].capabilities = *((uint16_t *) (ideBuffer + ATA_IDENT_CAPABILITIES));
				devices[count].commandSets  = *((uint32_t *) (ideBuffer + ATA_IDENT_COMMANDSETS));

				// Get size
				if (devices[count].commandSets & (1 << 26)) {
					// Device uses 48-bit addressing
					devices[count].size = *((uint32_t *) (ideBuffer + ATA_IDENT_MAX_LBA_EXT));
				} else {
					// Device uses CHS or 28-bit addressing
					devices[count].size = *((uint32_t *) (ideBuffer + ATA_IDENT_MAX_LBA));
				}

				// String indicates model of device
				for (int k = 0; k < 40; k += 2) {
					devices[count].model[k]		= ideBuffer[ATA_IDENT_MODEL + k + 1];
					devices[count].model[k + 1] = ideBuffer[ATA_IDENT_MODEL + k];
				}
				devices[count].model[40] = '\0';

				count++;
			}

		// 4- Print Summary:
		for (int i = 0; i < 4; i++)
			if (devices[i].reserved)
				printf("Slot %d: found %s Drive %dMB - %s\n", i,
					devices[i].type == 0? "ATA" : "ATAPI",
					devices[i].size / 2048,
					devices[i].model);
	}

	/* This function reads/writes sectors from ATA drives.
	 * - drive is the drive number, which can be from 0 to 3.
	 * - lba is the LBA address which allows us to access disks up to 2TB.
	 * - numsects is the number of sectors to be read. It's a char, as reading more than 256 sectors immediately may
	 *   cause performance issues. If numsects is 0, the ATA controller will know that we want 256 sectors.
	 */
	uint8_t accessATA(bool writing, uint8_t drive, uint32_t lba, uint8_t numsects, char *buffer) {
		uint8_t  lba_mode /* 0: CHS, 1: LBA28, 2: LBA48 */, dma /* 0: No DMA, 1: DMA */, cmd;
		uint8_t  lba_io[6];
		uint32_t channel = devices[drive].channel; // Read the channel
		uint32_t slavebit = devices[drive].drive; // Read the drive (master/slave)
		uint32_t bus = channels[channel].base; // Bus base, like 0x1f0, which is also a data port
		uint32_t words = 256; // Almost every ATA drive has a sector size of 512 bytes.
		uint16_t cyl, i;
		uint8_t  head, sect, err;

		write(channel, ATA_REG_CONTROL, channels[channel].nIEN = (irqInvoked = 0) + 0x02);

		// Select one from LBA28, LBA48 or CHS
		if (lba >= 0x10000000) { // Sure drive should support LBA in this case, or you are giving a wrong LBA.
			// LBA48:
			lba_mode  = 2;
			lba_io[0] = (lba & 0x000000FF) >> 0;
			lba_io[1] = (lba & 0x0000FF00) >> 8;
			lba_io[2] = (lba & 0x00FF0000) >> 16;
			lba_io[3] = (lba & 0xFF000000) >> 24;
			lba_io[4] = 0; // LBA28 is an integer, so 32 bits are enough to access 2 TB
			lba_io[5] = 0; // LBA28 is an integer, so 32 bits are enough to access 2 TB
			head	  = 0; // Lower 4-bits of HDDEVSEL are not used here
		} else if (devices[drive].capabilities & 0x200) { // Drive supports LBA
			// LBA28:
			lba_mode  = 1;
			lba_io[0] = (lba & 0x00000FF) >> 0;
			lba_io[1] = (lba & 0x000FF00) >> 8;
			lba_io[2] = (lba & 0x0FF0000) >> 16;
			lba_io[3] = 0; // These registers are not used here.
			lba_io[4] = 0;
			lba_io[5] = 0;
			head	  = (lba & 0xF000000) >> 24;
		} else {
			// CHS:
			lba_mode  = 0;
			sect	  = (lba % 63) + 1;
			cyl		  = (lba + 1 - sect) / (16 * 63);
			lba_io[0] = sect;
			lba_io[1] = (cyl >> 0) & 0xFF;
			lba_io[2] = (cyl >> 8) & 0xFF;
			lba_io[3] = 0;
			lba_io[4] = 0;
			lba_io[5] = 0;
			head	  = (lba + 1 - sect) % (16 * 63) / (63); // Head number is written to HDDEVSEL lower 4 bits
		}

		// See whether drive supports DMA
		dma = 0; // Drive doesn't support DMA

		// Wait if the drive is busy
		while (read(channel, ATA_REG_STATUS) & ATA_SR_BSY);

		// Select drive from the controller
		if (lba_mode == 0)
			write(channel, ATA_REG_HDDEVSEL, 0xA0 | (slavebit << 4) | head); // Drive & CHS
		else
			write(channel, ATA_REG_HDDEVSEL, 0xE0 | (slavebit << 4) | head); // Drive & LBA

		// Write parameters
		if (lba_mode == 2) {
			write(channel, ATA_REG_SECCOUNT1, 0);
			write(channel, ATA_REG_LBA3, lba_io[3]);
			write(channel, ATA_REG_LBA4, lba_io[4]);
			write(channel, ATA_REG_LBA5, lba_io[5]);
		}
		write(channel, ATA_REG_SECCOUNT0, numsects);
		write(channel, ATA_REG_LBA0, lba_io[0]);
		write(channel, ATA_REG_LBA1, lba_io[1]);
		write(channel, ATA_REG_LBA2, lba_io[2]);

		// Select the command and send it
		if (!writing) {
			if (dma == 0) {
				if (lba_mode == 0)
					cmd = ATA_CMD_READ_PIO;
				else if (lba_mode == 1)
					cmd = ATA_CMD_READ_PIO;
				else if (lba_mode == 2)
					cmd = ATA_CMD_READ_PIO_EXT;
			} else if (dma == 1) {
				if (lba_mode == 0)
					cmd = ATA_CMD_READ_DMA;
				else if (lba_mode == 1)
					cmd = ATA_CMD_READ_DMA;
				else if (lba_mode == 2)
					cmd = ATA_CMD_READ_DMA_EXT;
			}
		} else {
			if (dma == 0) {
				if (lba_mode == 0)
					cmd = ATA_CMD_WRITE_PIO;
				else if (lba_mode == 1)
					cmd = ATA_CMD_WRITE_PIO;
				else if (lba_mode == 2)
					cmd = ATA_CMD_WRITE_PIO_EXT;
			} else if (dma == 1) {
				if (lba_mode == 0)
					cmd = ATA_CMD_WRITE_DMA;
				else if (lba_mode == 1)
					cmd = ATA_CMD_WRITE_DMA;
				else if (lba_mode == 2)
					cmd = ATA_CMD_WRITE_DMA_EXT;
			}
		}
		
		write(channel, ATA_REG_COMMAND, cmd); // Send the command
		if (dma) {
			return -1; // No support
		} else if (!writing) {
			// PIO read
			for (i = 0; i < numsects; i++) {
				if ((err = polling(channel, 1)))
					return err;
				// uint64_t es;
				// asm volatile("mov %%es, %0" : "=r"(es));
				// asm volatile("mov %%ax, %%es" :: "a"(selector));
				asm volatile("rep insw" :: "c"(words), "d"(bus), "D"(buffer)); // Receive Data.
				// asm volatile("mov %0, %%es" :: "r"(es));
				buffer += (words * 2);
			}
		} else {
			// PIO write
			for (i = 0; i < numsects; i++) {
				polling(channel, 0);
				// uint64_t ds;
				// asm volatile("mov %%ds, %0" : "=r"(ds));
				// asm volatile("mov %%ax, %%ds" :: "a"(selector));
				asm volatile("rep outsw" :: "c"(words), "d"(bus), "S"(buffer)); // Send Data
				// asm volatile("mov %0, %%ds" :: "r"(ds));
				buffer += (words * 2);
			}
			write(channel, ATA_REG_COMMAND, ((int[]) {ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH, ATA_CMD_CACHE_FLUSH_EXT})[lba_mode]);
			polling(channel, 0);
		}

		return 0;
	}

	uint8_t printError(uint32_t drive, uint8_t err) {
		if (err == 0)
			return err;

		printf("IDE:");
		if (err == 1) {
			printf("- Device Fault\n");
			err = 19;
		} else if (err == 2) {
			uint8_t st = read(devices[drive].channel, ATA_REG_ERROR);
			if (st & ATA_ER_AMNF) {
				printf("- No Address Mark Found\n");
				err = 7;
			}
			if (st & ATA_ER_TK0NF) {
				printf("- No Media or Media Error\n");
				err = 3;
			}
			if (st & ATA_ER_ABRT) {
				printf("- Command Aborted\n");
				err = 20;
			}
			if (st & ATA_ER_MCR) {
				printf("- No Media or Media Error\n");
				err = 3;
			}
			if (st & ATA_ER_IDNF) {
				printf("- ID mark not Found\n");
				err = 21;
			}
			if (st & ATA_ER_MC) {
				printf("- No Media or Media Error\n");
				err = 3;
			}
			if (st & ATA_ER_UNC) {
				printf("- Uncorrectable Data Error\n");
				err = 22;
			}
			if (st & ATA_ER_BBK) {
				printf("- Bad Sectors\n");
				err = 13;
			}
		} else if (err == 3) {
			printf("- Reads Nothing\n");
			err = 23;
		} else if (err == 4) {
			printf("- Write Protected\n");
			err = 8;
		}
		printf("- [%s %s] %s\n",
			(const char *[]) {"Primary", "Secondary"}[devices[drive].channel], // Use the channel as an index into the array
			(const char *[]) {"Master", "Slave"}[devices[drive].drive], // Same as above, using the drive
			devices[drive].model);

		return err;
	}

	uint8_t polling(uint8_t channel, uint32_t advanced_check) {
		for (uint8_t i = 0; i < 4; i++)
			read(channel, ATA_REG_ALTSTATUS);

		while (read(channel, ATA_REG_STATUS) & ATA_SR_BSY);

		if (advanced_check) {
			uint8_t state = read(channel, ATA_REG_STATUS); // Read status register

			if (state & ATA_SR_DF)
				return 1; // Device fault

			if (state & ATA_SR_ERR)
				return 2; // Error

			if ((state & ATA_SR_DRQ) == 0)
				return 3; // DRQ should be set
		}

		return 0; // No Error.
	}

	uint8_t read(uint8_t channel, uint8_t reg) {
		uint8_t result = 0;
		if (reg > 0x07 && reg < 0x0C)
			write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
		if (reg < 0x08)
			result = inb(channels[channel].base + reg - 0x00);
		else if (reg < 0x0C)
			result = inb(channels[channel].base + reg - 0x06);
		else if (reg < 0x0E)
			result = inb(channels[channel].ctrl + reg - 0x0A);
		else if (reg < 0x16)
			result = inb(channels[channel].bmide + reg - 0x0E);
		if (reg > 0x07 && reg < 0x0C)
			write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
		return result;
	}

	void write(uint8_t channel, uint8_t reg, uint8_t data) {
		if (reg > 0x07 && reg < 0x0C)
			write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
		if (reg < 0x08)
			outb(channels[channel].base + reg - 0x00, data);
		else if (reg < 0x0C)
			outb(channels[channel].base + reg - 0x06, data);
		else if (reg < 0x0E)
			outb(channels[channel].ctrl + reg - 0x0A, data);
		else if (reg < 0x16)
			outb(channels[channel].bmide + reg - 0x0E, data);
		if (reg > 0x07 && reg < 0x0C)
			write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	}

	void readBuffer(uint8_t channel, uint8_t reg, void *buffer, uint32_t quads) {
		if (reg > 0x07 && reg < 0x0C)
			write(channel, ATA_REG_CONTROL, 0x80 | channels[channel].nIEN);
		if (reg < 0x08)
			insl(channels[channel].base + reg - 0x00, buffer, quads);
		else if (reg < 0x0C)
			insl(channels[channel].base + reg - 0x06, buffer, quads);
		else if (reg < 0x0E)
			insl(channels[channel].ctrl + reg - 0x0A, buffer, quads);
		else if (reg < 0x16)
			insl(channels[channel].bmide + reg - 0x0E, buffer, quads);
		if (reg > 0x07 && reg < 0x0C)
			write(channel, ATA_REG_CONTROL, channels[channel].nIEN);
	}

	void waitIRQ() {
		printf("irqInvoked: %d\n", irqInvoked);
		while (!irqInvoked);
		printf(": irqInvoked: %d 0x%lx\n", irqInvoked, &irqInvoked);
		irqInvoked = false;
	}

	uint8_t readATAPI(uint8_t drive, uint32_t lba, char *buffer) {
		uint32_t channel  = devices[drive].channel;
		uint32_t slavebit = devices[drive].drive;
		uint32_t bus      = channels[channel].base;
		uint32_t words    = 1024; // ATAPI drives have a sector size of 2048 bytes.
		uint8_t err;

		write(channel, ATA_REG_CONTROL, channels[channel].nIEN = irqInvoked = false);

		atapiPacket[ 0] = ATAPI_CMD_READ;
		atapiPacket[ 1] = 0x0;
		atapiPacket[ 2] = (lba >> 24) & 0xFF;
		atapiPacket[ 3] = (lba >> 16) & 0xFF;
		atapiPacket[ 4] = (lba >> 8) & 0xFF;
		atapiPacket[ 5] = (lba >> 0) & 0xFF;
		atapiPacket[ 6] = 0x0;
		atapiPacket[ 7] = 0x0;
		atapiPacket[ 8] = 0x0;
		atapiPacket[ 9] = 1;
		atapiPacket[10] = 0x0;
		atapiPacket[11] = 0x0;

		write(channel, ATA_REG_HDDEVSEL, slavebit << 4);
		for (uint8_t i = 0; i < 4; i++)
			read(channel, ATA_REG_ALTSTATUS);
		write(channel, ATA_REG_FEATURES, 0);
		write(channel, ATA_REG_LBA1, (words * 2) & 0xFF);
		write(channel, ATA_REG_LBA2, (words * 2) >> 8);
		write(channel, ATA_REG_COMMAND, ATA_CMD_PACKET);

		if ((err = polling(channel, 1)))
			return err;

		asm("rep outsw" :: "c"(6), "d"(bus), "S"(atapiPacket));

		waitIRQ();
		if ((err = polling(channel, 1)))
			return err;
		asm("rep insw" :: "c"(words), "d"(bus), "D"(buffer));
		buffer += words * 2;

		waitIRQ();
		while (read(channel, ATA_REG_STATUS) & (ATA_SR_BSY | ATA_SR_DRQ));
		return 0;
	}
}
