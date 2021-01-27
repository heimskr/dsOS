#pragma once

// Credit goes to https://wiki.osdev.org/AHCI

#include <cstddef>
#include <cstdint>

#include "hardware/ATA.h"
#include "hardware/PCI.h"

namespace DsOS::AHCI {
	constexpr uint32_t SIG_ATA   = 0x00000101; // SATA drive
	constexpr uint32_t SIG_ATAPI = 0xEB140101; // SATAPI drive
	constexpr uint32_t SIG_SEMB  = 0xC33C0101; // Enclosure management bridge
	constexpr uint32_t SIG_PM    = 0x96690101; // Port multiplier

	constexpr uint8_t HBA_PORT_IPM_ACTIVE  = 1;
	constexpr uint8_t HBA_PORT_DET_PRESENT = 3;

	constexpr uint32_t AHCI_BASE = 0x400000; // 4M

	constexpr uint16_t HBA_PxCMD_ST  = 0x0001; // Start
	constexpr uint16_t HBA_PxCMD_FRE = 0x0010; // FIS Receive Enable
	constexpr uint16_t HBA_PxCMD_FR  = 0x4000; // FIS Receive Running
	constexpr uint16_t HBA_PxCMD_CR  = 0x8000; // Command List Running

	constexpr uint32_t HBA_PxIS_TFES = 1 << 30;

	constexpr uint8_t ATA_DEV_BUSY = 0x80;
	constexpr uint8_t ATA_DEV_DRQ  = 0x08;

	enum class FISType: unsigned char {
		RegH2D      = 0x27, // Register FIS - host to device
		RegD2H      = 0x34, // Register FIS - device to host
		DMAActivate = 0x39, // DMA activate FIS - device to host
		DMASetup    = 0x41, // DMA setup FIS - bidirectional
		Data        = 0x46, // Data FIS - bidirectional
		BIST        = 0x58, // BIST activate FIS - bidirectional
		PIOSetup    = 0x5f, // PIO setup FIS - device to host
		DevBits     = 0xa1, // Set device bits FIS - device to host
	};

	enum class DeviceType {
		Null           = 0,
		SATA           = 1,
		SEMB           = 2,
		PortMultiplier = 3,
		SATAPI         = 4
	};

	extern const char *deviceTypes[5];

	struct FISRegH2D { // Host to device
		// DWORD 0
		FISType type = FISType::RegH2D;

		uint8_t pmport: 4;   // Port multiplier
		uint8_t rsv0: 3;     // Reserved
		uint8_t c: 1;        // 1: Command, 0: Control

		ATA::Command command;     // Command register
		uint8_t featureLow;  // Feature register, 7:0

		// DWORD 1
		uint8_t lba0;        // LBA low register, 7:0
		uint8_t lba1;        // LBA mid register, 15:8
		uint8_t lba2;        // LBA high register, 23:16
		uint8_t device;	     // Device register

		// DWORD 2
		uint8_t lba3;        // LBA register, 31:24
		uint8_t lba4;        // LBA register, 39:32
		uint8_t lba5;        // LBA register, 47:40
		uint8_t featureHigh; // Feature register, 15:8

		// DWORD 3
		uint8_t countLow;    // Count register, 7:0
		uint8_t countHigh;   // Count register, 15:8
		uint8_t icc;         // Isochronous command completion
		uint8_t control;     // Control register

		// DWORD 4
		uint32_t rsv1;      // Reserved
	};

	struct FISRegD2H { // Device to host
		// DWORD 0
		FISType type/*  = FISType::RegD2H */;

		uint8_t pmport: 4;    // Port multiplier
		uint8_t rsv0: 2;      // Reserved
		bool interrupt: 1;    // Interrupt bit
		uint8_t rsv1: 1;      // Reserved

		uint8_t status;       // Status register
		uint8_t error;        // Error register

		// DWORD 1
		uint8_t lba0;         // LBA low register, 7:0
		uint8_t lba1;         // LBA mid register, 15:8
		uint8_t lba2;         // LBA high register, 23:16
		uint8_t device;       // Device register

		// DWORD 2
		uint8_t lba3;         // LBA register, 31:24
		uint8_t lba4;         // LBA register, 39:32
		uint8_t lba5;         // LBA register, 47:40
		uint8_t rsv2;         // Reserved

		// DWORD 3
		uint8_t countLow;     // Count register, 7:0
		uint8_t countHigh;    // Count register, 15:8
		uint16_t rsv3;        // Reserved

		// DWORD 4
		uint32_t rsv4;        // Reserved
	};

	class FISData { // Bidirectional
		// DWORD 0
		FISType type = FISType::Data;

		uint8_t pmport: 4; // Port multiplier
		uint8_t rsv0: 4;   // Reserved

		uint16_t rsv1;     // Reserved

		// DWORD 1 ~ N
		uint32_t data[1];  // Payload
	};

	struct FISPIOSetup { // Device to host
		// DWORD 0
		FISType type = FISType::PIOSetup;

		uint8_t pmport:4;     // Port multiplier
		uint8_t rsv0: 1;      // Reserved
		uint8_t direction: 1; // Data transfer direction (1 = device to host)
		bool interrupt: 1; // Interrupt bit
		uint8_t rsv1: 1;      // Reserved

		uint8_t status;       // Status register
		uint8_t error;        // Error register

		// DWORD 1
		uint8_t lba0;         // LBA low register, 7:0
		uint8_t lba1;         // LBA mid register, 15:8
		uint8_t lba2;         // LBA high register, 23:16
		uint8_t device;       // Device register

		// DWORD 2
		uint8_t lba3;         // LBA register, 31:24
		uint8_t lba4;         // LBA register, 39:32
		uint8_t lba5;         // LBA register, 47:40
		uint8_t rsv2;         // Reserved

		// DWORD 3
		uint8_t countl;       // Count register, 7:0
		uint8_t counth;       // Count register, 15:8
		uint8_t rsv3;         // Reserved
		uint8_t e_status;     // New value of status register

		// DWORD 4
		uint16_t tc;          // Transfer count
		uint16_t rsv4;        // Reserved
	};

	struct FISDMASetup { // Device to host
		// DWORD 0
		FISType type = FISType::DMASetup;

		uint8_t pmport: 4;        // Port multiplier
		uint8_t rsv0: 1;          // Reserved
		uint8_t direction: 1;     // Data transfer direction, 1 - device to host
		bool interrupt: 1;     // Interrupt bit
		bool autoActivate: 1;  // Auto-activate. Specifies if DMA Activate FIS is needed

		uint16_t rsv1;            // Reserved

		// DWORDs 1 & 2
		uint64_t dmaBufferID;     // DMA Buffer Identifier. Used to identify DMA buffer in host memory.
		                          // The SATA spec says host specific and not in spec. Trying AHCI spec might work.

		// DWORD 3
		uint32_t rsv2;            // Reserved

		// DWORD 4
		uint32_t dmaBufferOffset; // Byte offset into buffer. First 2 bits must be 0.

		// DWORD 5
		uint32_t transferCount;   // Number of bytes to transfer. Bit 0 must be 0.

		// DWORD 6
		uint32_t rsv3;            // Reserved
	};

	struct HBAMemory;

	struct HBAPort {
		volatile uint32_t clb;       // 0x00: Command list base address, 1 KiB-aligned
		volatile uint32_t clbu;      // 0x04: Command list base address upper 32 bits
		volatile uint32_t fb;        // 0x08: FIS base address, 256-byte aligned
		volatile uint32_t fbu;       // 0x0c: FIS base address upper 32 bits
		volatile uint32_t is;        // 0x10: Interrupt status
		volatile uint32_t ie;        // 0x14: Interrupt enable
		volatile uint32_t cmd;       // 0x18: Command and status
		volatile uint32_t rsv0;      // 0x1c: Reserved
		volatile uint32_t tfd;       // 0x20: Task file data
		volatile uint32_t sig;       // 0x24: Signature
		volatile uint32_t ssts;      // 0x28: SATA status (SCR0:SStatus)
		volatile uint32_t sctl;      // 0x2c: SATA control (SCR2:SControl)
		volatile uint32_t serr;      // 0x30: SATA error (SCR1:SError)
		volatile uint32_t sact;      // 0x34: SATA active (SCR3:SActive)
		volatile uint32_t ci;        // 0x38: Command issue
		volatile uint32_t sntf;      // 0x3c: SATA notification (SCR4:SNotification)
		volatile uint32_t fbs;       // 0x40: FIS-based switch control
		volatile uint32_t devslp;    // 0x44–0x6f: Device sleep
		volatile uint32_t vendor[4]; // 0x70–0x7f: vendor specific

		DeviceType identifyDevice() volatile;
		int getCommandSlot(volatile HBAMemory &) volatile;
		void rebase(volatile HBAMemory &) volatile;
		void start() volatile;
		void stop() volatile;
		void setCLB(void *) volatile;
		void * getCLB() const volatile;
		void setFB(void *) volatile;
		void * getFB() const volatile;
	};

	struct HBAFIS {
		// 0x00
		FISDMASetup dsfis; // DMA Setup FIS
		uint8_t pad0[4];

		// 0x20
		FISPIOSetup psfis; // PIO Setup FIS
		uint8_t pad1[12];

		// 0x40
		FISRegD2H rfis; // Register – Device to Host FIS
		uint8_t pad2[4];

		// 0x58
		uint64_t sdbfis; // Set Device Bit FIS

		// 0x60
		uint8_t ufis[64];

		// 0xa0
		uint8_t reserved[0x100 - 0xa0];
	};

	struct HBAMemory {
		// 0x00 - 0x2B, Generic Host Control
		uint32_t cap;     // 0x00: Host capability
		uint32_t ghc;     // 0x04: Global host control
		uint32_t is;      // 0x08: Interrupt status
		uint32_t pi;      // 0x0c: Port implemented
		uint32_t vs;      // 0x10: Version
		uint32_t ccc_ctl; // 0x14: Command completion coalescing control
		uint32_t ccc_pts; // 0x18: Command completion coalescing ports
		uint32_t em_loc;  // 0x1c: Enclosure management location
		uint32_t em_ctl;  // 0x20: Enclosure management control
		uint32_t cap2;    // 0x24: Host capabilities extended
		uint32_t bohc;    // 0x28: BIOS/OS handoff control and status

		// 0x2c–0x9f: Reserved
		uint8_t rsv0[0xa0 - 0x2c];

		// 0xa0–0xff: Vendor specific registers
		uint8_t vendor[0x100 - 0xa0];

		// 0x100–0x10ff: Port control registers
		HBAPort ports[32];

		void probe() volatile;
	};

	struct HBACommandHeader {
		// DWORD 0
		uint8_t cfl: 5;       // Command FIS length in DWORDS, 2 ~ 16
		bool atapi: 1;        // ATAPI
		bool write: 1;        // Write (1: H2D, 0: D2H)
		bool prefetchable: 1; // Prefetchable

		bool reset: 1;     // Reset
		bool bist: 1;      // BIST
		bool clearBusy: 1; // Clear busy upon R_OK
		uint8_t rsv0: 1;   // Reserved
		uint8_t pmport: 4; // Port multiplier port

		uint16_t prdtl; // Physical region descriptor table length in entries

		// DWORD 1
		volatile uint32_t prdbc; // Physical region descriptor byte count transferred

		// DWORDs 2 & 3
		uint32_t ctba;  // Command table descriptor base address
		uint32_t ctbau; // Command table descriptor base address upper 32 bits

		// DWORDs 4 - 7
		uint32_t rsv1[4]; // Reserved

		void setCTBA(void *) volatile;
		void * getCTBA() const volatile;
	};

	struct HBAPRDTEntry {
		uint32_t dba;      // Data base address
		uint32_t dbaUpper; // Data base address upper 32 bits
		uint32_t rsv0;     // Reserved

		// DWORD 3
		uint32_t dbc: 22;  // Byte count, 4MiB max
		uint32_t rsv1: 9;  // Reserved
		bool interrupt: 1; // Interrupt on completion
	};

	struct HBACommandTable {
		// 0x00: Command FIS
		uint8_t cfis[64];

		// 0x40: ATAPI command, 12 or 16 bytes
		uint8_t atapiCommand[16];

		// 0x50: Reserved
		uint8_t rsv[48];

		// 0x80: Physical region descriptor table entries, 0 ~ 65535
		HBAPRDTEntry prdtEntry[1];
	};

	extern PCI::Device *controller;
	extern volatile HBAMemory *abar;
}
