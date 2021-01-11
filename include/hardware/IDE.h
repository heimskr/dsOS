#pragma once

#include <cstddef>
#include <cstdint>

extern bool irqInvoked;

namespace DsOS::IDE {
	constexpr size_t SECTOR_SIZE = 512;

	// Status codes
	constexpr uint8_t ATA_SR_BSY = 0x80;
	constexpr uint8_t ATA_SR_DRDY = 0x40;
	constexpr uint8_t ATA_SR_DF = 0x20;
	constexpr uint8_t ATA_SR_DSC = 0x10;
	constexpr uint8_t ATA_SR_DRQ = 0x08;
	constexpr uint8_t ATA_SR_CORR = 0x04;
	constexpr uint8_t ATA_SR_IDX = 0x02;
	constexpr uint8_t ATA_SR_ERR = 0x01;

	// Error codes
	constexpr uint8_t ATA_ER_BBK = 0x80;
	constexpr uint8_t ATA_ER_UNC = 0x40;
	constexpr uint8_t ATA_ER_MC = 0x20;
	constexpr uint8_t ATA_ER_IDNF = 0x10;
	constexpr uint8_t ATA_ER_MCR = 0x08;
	constexpr uint8_t ATA_ER_ABRT = 0x04;
	constexpr uint8_t ATA_ER_TK0NF = 0x02;
	constexpr uint8_t ATA_ER_AMNF = 0x01;

	// ATA commands
	constexpr uint8_t ATA_CMD_READ_PIO = 0x20;
	constexpr uint8_t ATA_CMD_READ_PIO_EXT = 0x24;
	constexpr uint8_t ATA_CMD_READ_DMA = 0xc8;
	constexpr uint8_t ATA_CMD_READ_DMA_EXT = 0x25;
	constexpr uint8_t ATA_CMD_WRITE_PIO = 0x30;
	constexpr uint8_t ATA_CMD_WRITE_PIO_EXT = 0x34;
	constexpr uint8_t ATA_CMD_WRITE_DMA = 0xca;
	constexpr uint8_t ATA_CMD_WRITE_DMA_EXT = 0x35;
	constexpr uint8_t ATA_CMD_CACHE_FLUSH = 0xe7;
	constexpr uint8_t ATA_CMD_CACHE_FLUSH_EXT = 0xea;
	constexpr uint8_t ATA_CMD_PACKET = 0xa0;
	constexpr uint8_t ATA_CMD_IDENTIFY_PACKET = 0xa1;
	constexpr uint8_t ATA_CMD_IDENTIFY = 0xec;

	constexpr uint8_t ATAPI_CMD_READ = 0xa8;
	constexpr uint8_t ATAPI_CMD_EJECT = 0x1b;

	// identification
	constexpr uint8_t ATA_IDENT_DEVICETYPE = 0;
	constexpr uint8_t ATA_IDENT_CYLINDERS = 2;
	constexpr uint8_t ATA_IDENT_HEADS = 6;
	constexpr uint8_t ATA_IDENT_SECTORS = 12;
	constexpr uint8_t ATA_IDENT_SERIAL = 20;
	constexpr uint8_t ATA_IDENT_MODEL = 54;
	constexpr uint8_t ATA_IDENT_CAPABILITIES = 98;
	constexpr uint8_t ATA_IDENT_FIELDVALID = 106;
	constexpr uint8_t ATA_IDENT_MAX_LBA = 120;
	constexpr uint8_t ATA_IDENT_COMMANDSETS = 164;
	constexpr uint8_t ATA_IDENT_MAX_LBA_EXT = 200;

	// ATA-ATAPI Task-File
	constexpr uint8_t ATA_REG_DATA = 0x00;
	constexpr uint8_t ATA_REG_ERROR = 0x01;
	constexpr uint8_t ATA_REG_FEATURES = 0x01;
	constexpr uint8_t ATA_REG_SECCOUNT0 = 0x02;
	constexpr uint8_t ATA_REG_LBA0 = 0x03;
	constexpr uint8_t ATA_REG_LBA1 = 0x04;
	constexpr uint8_t ATA_REG_LBA2 = 0x05;
	constexpr uint8_t ATA_REG_HDDEVSEL = 0x06;
	constexpr uint8_t ATA_REG_COMMAND = 0x07;
	constexpr uint8_t ATA_REG_STATUS = 0x07;
	constexpr uint8_t ATA_REG_SECCOUNT1 = 0x08;
	constexpr uint8_t ATA_REG_LBA3 = 0x09;
	constexpr uint8_t ATA_REG_LBA4 = 0x0a;
	constexpr uint8_t ATA_REG_LBA5 = 0x0b;
	constexpr uint8_t ATA_REG_CONTROL = 0x0c;
	constexpr uint8_t ATA_REG_ALTSTATUS = 0x0c;
	constexpr uint8_t ATA_REG_DEVADDRESS = 0x0d;

	// Channels
	constexpr uint8_t ATA_PRIMARY = 0x00;
	constexpr uint8_t ATA_SECONDARY = 0x01;

	// Directions
	constexpr uint8_t ATA_READ = 0x00;
	constexpr uint8_t ATA_WRITE = 0x01;

	constexpr uint8_t IDE_ATA = 0x00;
	constexpr uint8_t IDE_ATAPI = 0x01;

	constexpr uint8_t ATA_MASTER = 0x00;
	constexpr uint8_t ATA_SLAVE = 0x01;

	struct Device {
		bool reserved; // 0 (Empty) or 1 (This Drive really exists).
		uint8_t  channel; // 0 (Primary Channel) or 1 (Secondary Channel).
		uint8_t  drive; // 0 (Master Drive) or 1 (Slave Drive).
		uint16_t type; // 0: ATA, 1:ATAPI.
		uint16_t signature; // Drive Signature
		uint16_t capabilities; // Features.
		uint32_t commandSets; // Command Sets Supported.
		uint32_t size; // Size in Sectors.
		char     model[41]; // Model in string.
	};

	struct ChannelRegisters {
		uint16_t base;  // I/O Base.
		uint16_t ctrl;  // Control Base
		uint16_t bmide; // Bus Master IDE
		uint8_t  nIEN;  // nIEN (No Interrupt);
	};

	extern Device devices[4];

	void init();
	int readSectors(uint8_t drive, uint8_t numsects, uint32_t lba, char *buffer);
	int writeSectors(uint8_t drive, uint8_t numsects, uint32_t lba, const void *buffer);
	int readBytes(uint8_t drive, size_t bytes, size_t offset, void *buffer);
	int writeBytes(uint8_t drive, size_t bytes, size_t offset, const void *buffer);

	void init(uint32_t bar0, uint32_t bar1, uint32_t bar2, uint32_t bar3, uint32_t bar4);
	uint8_t read(uint8_t channel, uint8_t reg);
	void write(uint8_t channel, uint8_t reg, uint8_t data);
	void readBuffer(uint8_t channel, uint8_t reg, void *buffer, uint32_t quads);
	uint8_t polling(uint8_t channel, uint32_t advanced_check);
	uint8_t printError(uint32_t drive, uint8_t err);
	uint8_t accessATA(bool writing, uint8_t drive, uint32_t lba, uint8_t numsects, char *buffer);
	uint8_t readATAPI(uint8_t drive, uint32_t lba, char *buffer);
}