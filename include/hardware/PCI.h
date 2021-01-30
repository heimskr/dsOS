/*******************************************************************************
/
/	File:		PCI.h
/
/	Description:	Interface to the PCI bus.
/	For more information, see "PCI Local Bus Specification, Revision 2.1",
/	PCI Special Interest Group, 1995.
/
/	Copyright 1993-98, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

// Some code is from https://github.com/imgits/ShellcodeOS/blob/master/OS/pci/pci.h
// Some code is from https://github.com/fido2020/Lemon-OS/blob/master/Kernel/include/arch/x86_64/pci.h

#pragma once

#include <vector>

#include "Defs.h"

namespace DsOS::PCI {
	constexpr uint8_t MSI_CONTROL_64 = 1 << 7;
	constexpr uint32_t MSI_ADDRESS_BASE = 0xfee00000;

	struct BDF {
		uint32_t bus, device, function;
		BDF(uint32_t bus_, uint32_t device_, uint32_t function_): bus(bus_), device(device_), function(function_) {}
	};

	uint8_t  readByte(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset);
	uint16_t readWord(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset);
	uint32_t readInt (uint32_t bus, uint32_t device, uint32_t function, uint32_t offset);
	void writeByte(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset, uint8_t  val);
	void writeWord(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset, uint16_t val);
	void writeInt (uint32_t bus, uint32_t device, uint32_t function, uint32_t offset, uint32_t val);
	uint8_t  readByte(const volatile BDF &, uint32_t offset);
	uint16_t readWord(const volatile BDF &, uint32_t offset);
	uint32_t readInt (const volatile BDF &, uint32_t offset);
	void writeByte(const volatile BDF &, uint32_t offset, uint8_t  val);
	void writeWord(const volatile BDF &, uint32_t offset, uint16_t val);
	void writeInt (const volatile BDF &, uint32_t offset, uint32_t val);

	uint16_t getVendorID(uint32_t bus, uint32_t device, uint32_t function);
	uint16_t getVendorID(uint32_t bus, uint32_t device, uint32_t function);
	uint16_t getDeviceID(uint32_t bus, uint32_t device, uint32_t function);
	uint16_t getCommand(uint32_t bus, uint32_t device, uint32_t function);
	uint16_t getStatus(uint32_t bus, uint32_t device, uint32_t function);
	uint8_t getRevisionID(uint32_t bus, uint32_t device, uint32_t function);
	uint8_t getProgIF(uint32_t bus, uint32_t device, uint32_t function);
	uint8_t getBaseClass(uint32_t bus, uint32_t device, uint32_t function);
	uint8_t getSubclass(uint32_t bus, uint32_t device, uint32_t function);
	uint8_t getCacheLineSize(uint32_t bus, uint32_t device, uint32_t function);
	uint8_t getLatencyTimer(uint32_t bus, uint32_t device, uint32_t function);
	uint8_t getHeaderType(uint32_t bus, uint32_t device, uint32_t function);
	uint8_t getBIST(uint32_t bus, uint32_t device, uint32_t function);

	struct IDs {
		uint32_t vendor;
		uint32_t device;
		uint32_t subvendor;
		uint32_t subdevice;
	};

	struct DeviceClass {
		uint32_t baseclass;
		uint32_t subclass;
		uint32_t progif;
	};

	struct HeaderNative { // type 0
		uint16_t vendorID;
		uint16_t deviceID;
		uint16_t command;
		uint16_t status;
		uint8_t revision;
		uint8_t progif;
		uint8_t subclass;
		uint8_t classCode;
		uint8_t cacheLineSize;
		uint8_t latencyTimer;
		uint8_t headerType;
		uint8_t bist;
		uint32_t bar0;
		uint32_t bar1;
		uint32_t bar2;
		uint32_t bar3;
		uint32_t bar4;
		uint32_t bar5;
		uint32_t cardbusCISPointer;
		uint16_t subsystemVendorID;
		uint16_t subsystemID;
		uint32_t expansionROMBaseAddress;
		uint8_t capabilitiesPointer;
		uint8_t reserved0[3];
		uint32_t reserved1;
		uint8_t interruptLine;
		uint8_t interruptPin;
		uint8_t minGrant;
		uint8_t maxLatency;
	} __attribute__((packed));

	struct HeaderPCItoPCI { // type 1
		uint16_t vendorID;
		uint16_t deviceID;
		uint16_t command;
		uint16_t status;
		uint8_t revision;
		uint8_t progif;
		uint8_t subclass;
		uint8_t classCode;
		uint8_t cacheLineSize;
		uint8_t latencyTimer;
		uint8_t headerType;
		uint8_t bist;
		uint32_t bar0;
		uint32_t bar1;
		uint8_t primaryBusNumber;
		uint8_t secondaryBusNumber;
		uint8_t subordinateBusNumber;
		uint8_t secondaryLatencyTimer;
		uint8_t ioBase;
		uint8_t ioLimit;
		uint16_t secondaryStatus;
		uint16_t memoryBase;
		uint16_t memoryLimit;
		uint16_t prefetchableMemoryBase;
		uint16_t prefetchableMemoryLimit;
		uint32_t prefetchableMemoryBaseUpper;
		uint32_t prefetchableMemoryLimitUpper;
		uint16_t ioBaseUpper;
		uint16_t ioLimitUpper;
		uint8_t capabilitiesPointer;
		uint8_t reserved0[3];
		uint32_t expansionROMBaseAddress;
		uint8_t interruptLine;
		uint8_t interruptPin;
		uint16_t bridgeControl;
	} __attribute__((packed));

	enum class Vector: uint8_t {
		Legacy = 1, // Legacy IRQ
		APIC   = 2, // I/O APIC
		MSI    = 4, // Message Signaled Interrupt
		Any    = 7, // (Legacy | APIC | MSI)
	};

	struct MSICapability {
		union {
			struct {
				uint32_t capID: 8;
				uint32_t nextCap: 8;     // Next capability
				uint32_t msiControl: 16; // MSI control register
			} __attribute__((packed));
			uint32_t register0;
		};

		union {
			uint32_t addressLow; // Interrupt Message Address Low
			uint32_t register1;
		};

		union {
			uint32_t data;        // MSI data
			uint32_t addressHigh; // Interrupt Message Address High (64-bit only)
			uint32_t register2;
		};

		union {
			uint32_t data64;    // MSI data when 64-bit capable
			uint32_t register3;
		};

		inline void setData(uint32_t data_) {
			if (msiControl & MSI_CONTROL_64)
				data64 = data_;
			else
				data = data_;
		}

		inline uint32_t getData() {
			return (msiControl & MSI_CONTROL_64)? data64 : data;
		}

		inline void setAddress(uint32_t cpu) {
			addressLow = MSI_ADDRESS_BASE | (cpu << 12);
			if (msiControl & MSI_CONTROL_64)
				addressHigh = 0;
		}
	} __attribute__((packed));

	struct Device {
		BDF bdf;
		uint8_t msiPointer = 0;
		bool msiCapable = false;
		MSICapability msiCapability;
		std::vector<uint8_t> capabilities;

		Device(const BDF &);

		void init();
		uint16_t readStatus();
		uintptr_t getBAR(uint8_t);
		uint16_t getCommand();
		void setCommand(uint16_t);
		uint8_t getInterruptLine();
		uint8_t getInterruptPin();
		uint8_t allocateVector(Vector);
		uint8_t allocateVector(uint8_t);
	};

	std::vector<BDF> getDevices(uint32_t base_class, uint32_t subclass);
	HeaderNative readNativeHeader(const BDF &);
	void readNativeHeader(const BDF &, HeaderNative &);
	Device * initDevice(const BDF &);
	void scan();
	size_t printDevices();

	using status_t = int32_t;

	// Offsets in PCI configuration space to the elements of the predefined header common to all header types
	constexpr uint8_t VENDOR_ID   = 0x00; // (2 bytes) Vendor ID
	constexpr uint8_t DEVICE_ID   = 0x02; // (2 bytes) Device ID
	constexpr uint8_t COMMAND     = 0x04; // (2 bytes) Command
	constexpr uint8_t STATUS      = 0x06; // (2 bytes) Status
	constexpr uint8_t REVISION    = 0x08; // (1 byte)  Revision ID
	constexpr uint8_t CLASS_API   = 0x09; // (1 byte)  Specific register interface type
	constexpr uint8_t CLASS_SUB   = 0x0a; // (1 byte)  Specific device function
	constexpr uint8_t CLASS_BASE  = 0x0b; // (1 byte)  Device type (display vs. network, etc)
	constexpr uint8_t LINE_SIZE   = 0x0c; // (1 byte)  Cache line size in 32 bit words
	constexpr uint8_t LATENCY     = 0x0d; // (1 byte)  Latency timer
	constexpr uint8_t HEADER_TYPE = 0x0e; // (1 byte)  Header type
	constexpr uint8_t BIST        = 0x0f; // (1 byte)  Built-in self-test
	constexpr uint8_t BAR0        = 0x10; // 4
	constexpr uint8_t BAR1        = 0x14; // 4
	constexpr uint8_t BAR2        = 0x18; // 4
	constexpr uint8_t BAR3        = 0x1c; // 4
	constexpr uint8_t BAR4        = 0x20; // 4
	constexpr uint8_t BAR5        = 0x24; // 4

	// Offsets in PCI configuration space to the elements of the predefined header common to header types 0x00 and 0x01
	constexpr uint8_t BASE_REGISTERS = 0x10; // Base registers (size varies)
	constexpr uint8_t INTERRUPT_LINE = 0x3c; // (1 byte) Interrupt line
	constexpr uint8_t INTERRUPT_PIN  = 0x3d; // (1 byte) Interrupt pin

	// Offsets in PCI configuration space to the elements of header type 0x00
	constexpr uint8_t CARDBUS_CIS         = 0x28; // (4 bytes) CardBus CIS (Card Information Structure) pointer (see PCMCIA v2.10 Spec)
	constexpr uint8_t SUBSYSTEM_VENDOR_ID = 0x2c; // (2 bytes) Subsystem (add-in card) vendor ID
	constexpr uint8_t SUBSYSTEM_ID        = 0x2e; // (2 bytes) Subsystem (add-in card) ID
	constexpr uint8_t ROM_BASE            = 0x30; // (4 bytes) Expansion rom base address
	constexpr uint8_t CAPABILITIES_PTR    = 0x34; // (1 byte)  Pointer to the start of the capabilities list
	constexpr uint8_t MIN_GRANT           = 0x3e; // (1 byte)  Burst period @ 33 MHz
	constexpr uint8_t MAX_LATENCY         = 0x3f; // (1 byte)  How often PCI access needed

	// Offsets in PCI configuration space to the elements of header type 0x01 (PCI-to-PCI bridge)
	constexpr uint8_t PRIMARY_BUS                       = 0x18; // 1 byte
	constexpr uint8_t SECONDARY_BUS                     = 0x19; // 1 byte
	constexpr uint8_t SUBORDINATE_BUS                   = 0x1a; // 1 byte
	constexpr uint8_t SECONDARY_LATENCY                 = 0x1b; // 1 byte (latency of secondary bus)
	constexpr uint8_t IO_BASE                           = 0x1c; // 1 byte (IO base address register for secondary bus)
	constexpr uint8_t IO_LIMIT                          = 0x1d; // 1 byte
	constexpr uint8_t SECONDARY_STATUS                  = 0x1e; // 2 bytes
	constexpr uint8_t MEMORY_BASE                       = 0x20; // 2 bytes
	constexpr uint8_t MEMORY_LIMIT                      = 0x22; // 2 bytes
	constexpr uint8_t PREFETCHABLE_MEMORY_BASE          = 0x24; // 2 bytes
	constexpr uint8_t PREFETCHABLE_MEMORY_LIMIT         = 0x26; // 2 bytes
	constexpr uint8_t PREFETCHABLE_MEMORY_BASE_UPPER32  = 0x28;
	constexpr uint8_t PREFETCHABLE_MEMORY_LIMIT_UPPER32 = 0x2c;
	constexpr uint8_t IO_BASE_UPPER16                   = 0x30; // 2 bytes
	constexpr uint8_t IO_LIMIT_UPPER16                  = 0x32; // 2 bytes
	constexpr uint8_t SUB_VENDOR_ID_1                   = 0x34; // 2 bytes
	constexpr uint8_t SUB_DEVICE_ID_1                   = 0x36; // 2 bytes
	constexpr uint8_t BRIDGE_ROM_BASE                   = 0x38;
	constexpr uint8_t BRIDGE_CONTROL                    = 0x3e; // 2 bytes

	// PCI type 2 header offsets
	constexpr uint8_t CAPABILITIES_PTR_2  = 0x14; // 1 byte
	constexpr uint8_t SECONDARY_STATUS_2  = 0x16; // 2 bytes
	constexpr uint8_t PRIMARY_BUS_2       = 0x18; // 1 byte
	constexpr uint8_t SECONDARY_BUS_2     = 0x19; // 1 byte
	constexpr uint8_t SUBORDINATE_BUS_2   = 0x1a; // 1 byte
	constexpr uint8_t SECONDARY_LATENCY_2 = 0x1b; // 1 byte (latency of secondary bus)
	constexpr uint8_t MEMORY_BASE0_2      = 0x1c; // 4 bytes
	constexpr uint8_t MEMORY_LIMIT0_2     = 0x20; // 4 bytes
	constexpr uint8_t MEMORY_BASE1_2      = 0x24; // 4 bytes
	constexpr uint8_t MEMORY_LIMIT1_2     = 0x28; // 4 bytes
	constexpr uint8_t IO_BASE0_2          = 0x2c; // 4 bytes
	constexpr uint8_t IO_LIMIT0_2         = 0x30; // 4 bytes
	constexpr uint8_t IO_BASE1_2          = 0x34; // 4 bytes
	constexpr uint8_t IO_LIMIT1_2         = 0x38; // 4 bytes
	constexpr uint8_t BRIDGE_CONTROL_2    = 0x3e; // 2 bytes

	constexpr uint8_t SUB_VENDOR_ID_2     = 0x40; // 2 bytes
	constexpr uint8_t SUB_DEVICE_ID_2     = 0x42; // 2 bytes

	constexpr uint8_t CARD_INTERFACE_2    = 0x44; // ???

	// Values for the class_base field in the common header
	constexpr uint8_t EARLY                    = 0x00; // Built before class codes defined
	constexpr uint8_t MASS_STORAGE             = 0x01; // Mass storage_controller
	constexpr uint8_t NETWORK                  = 0x02; // Network controller
	constexpr uint8_t DISPLAY                  = 0x03; // Display controller
	constexpr uint8_t MULTIMEDIA               = 0x04; // Multimedia device
	constexpr uint8_t MEMORY                   = 0x05; // Memory controller
	constexpr uint8_t BRIDGE                   = 0x06; // Bridge controller
	constexpr uint8_t SIMPLE_COMMUNICATIONS    = 0x07; // Simple communications controller
	constexpr uint8_t BASE_PERIPHERAL          = 0x08; // Base system peripherals
	constexpr uint8_t INPUT                    = 0x09; // Input devices
	constexpr uint8_t DOCKING_STATION          = 0x0a; // Docking stations
	constexpr uint8_t PROCESSOR                = 0x0b; // Processors
	constexpr uint8_t SERIAL_BUS               = 0x0c; // Serial bus controllers
	constexpr uint8_t WIRELESS                 = 0x0d; // Wireless controllers
	constexpr uint8_t INTELLIGENT_IO           = 0x0e;
	constexpr uint8_t SATELLITE_COMMUNICATIONS = 0x0f;
	constexpr uint8_t ENCRYPTION_DECRYPTION    = 0x10;
	constexpr uint8_t DATA_ACQUISITION         = 0x11;

	constexpr uint8_t UNDEFINED = 0xff; // Not in any defined class

	// Values for the class_sub field for class_base = 0x00 (built before class codes were defined)
	constexpr uint8_t EARLY_NOT_VGA = 0x00; // All except VGA
	constexpr uint8_t EARLY_VGA     = 0x01; // VGA devices

	// Values for the class_sub field for class_base = 0x01 (mass storage)
	constexpr uint8_t SCSI               = 0x00; // SCSI controller
	constexpr uint8_t IDE                = 0x01; // IDE controller
	constexpr uint8_t FLOPPY             = 0x02; // Floppy disk controller
	constexpr uint8_t IPI                = 0x03; // IPI bus controller
	constexpr uint8_t RAID               = 0x04; // RAID controller
	constexpr uint8_t ATA                = 0x05; // ATA controller with ADMA interface
	constexpr uint8_t SATA               = 0x06; // Serial ATA controller
	constexpr uint8_t SAS                = 0x07; // Serial Attached SCSI controller
	constexpr uint8_t MASS_STORAGE_OTHER = 0x80; // Other mass storage controller

	// Values of the class_api field for class_base = 0x01 (mass storage), class_sub = 0x06 (Serial ATA controller)
	constexpr uint8_t SATA_OTHER = 0x00; // Vendor specific interface
	constexpr uint8_t SATA_AHCI  = 0x01; // AHCI interface

	// Values for the class_sub field for class_base = 0x02 (network)
	constexpr uint8_t ETHERNET      = 0x00; // Ethernet controller
	constexpr uint8_t TOKEN_RING    = 0x01; // Token Ring controller
	constexpr uint8_t FDDI          = 0x02; // FDDI controller
	constexpr uint8_t ATM           = 0x03; // ATM controller
	constexpr uint8_t ISDN          = 0x04; // ISDN controller
	constexpr uint8_t NETWORK_OTHER = 0x80; // Other network controller

	// Values for the class_sub field for class_base = 0x03 (display)
	constexpr uint8_t VGA           = 0x00; // VGA controller
	constexpr uint8_t XGA           = 0x01; // XGA controller
	constexpr uint8_t _3D           = 0x02; // 3D controller
	constexpr uint8_t DISPLAY_OTHER = 0x80; // Other display controller

	// Values for the class_sub field for class_base = 0x04 (multimedia device)
	constexpr uint8_t VIDEO            = 0x00;	 // Video
	constexpr uint8_t AUDIO            = 0x01;	 // Audio
	constexpr uint8_t TELEPHONY        = 0x02;	 // Computer telephony device
	constexpr uint8_t HD_AUDIO         = 0x03;	 // HD audio
	constexpr uint8_t MULTIMEDIA_OTHER = 0x80;	 // Other multimedia device

	// Values for the class_sub field for class_base = 0x05 (memory)
	constexpr uint8_t RAM          = 0x00; // RAM
	constexpr uint8_t FLASH        = 0x01; // Flash
	constexpr uint8_t MEMORY_OTHER = 0x80; // Other memory controller

	// Values for the class_sub field for class_base = 0x06 (bridge)
	constexpr uint8_t HOST               = 0x00; // Host bridge
	constexpr uint8_t ISA                = 0x01; // ISA bridge
	constexpr uint8_t EISA               = 0x02; // EISA bridge
	constexpr uint8_t MICROCHANNEL       = 0x03; // MicroChannel bridge
	constexpr uint8_t PCI                = 0x04; // PCI-to-PCI bridge
	constexpr uint8_t PCMCIA             = 0x05; // PCMCIA bridge
	constexpr uint8_t NUBUS              = 0x06; // NuBus bridge
	constexpr uint8_t CARDBUS            = 0x07; // CardBus bridge
	constexpr uint8_t RACEWAY            = 0x08; // RACEway bridge
	constexpr uint8_t BRIDGE_TRANSPARENT = 0x09; // PCI transparent
	constexpr uint8_t BRIDGE_INFINIBAND  = 0x0a; // Infiniband
	constexpr uint8_t BRIDGE_OTHER       = 0x80; // Other bridge device

	// Values for the class_sub field for class_base = 0x07 (simple communications controllers)
	constexpr uint8_t SERIAL                      = 0x00; // Serial port controller
	constexpr uint8_t PARALLEL                    = 0x01; // Parallel port
	constexpr uint8_t MULTIPORT_SERIAL            = 0x02; // Multiport serial controller
	constexpr uint8_t MODEM                       = 0x03; // Modem
	constexpr uint8_t SIMPLE_COMMUNICATIONS_OTHER = 0x80; // Other communications device

	// Values of the class_api field for class_base = 0x07 (simple communications), class_sub = 0x00 (serial port controller)
	constexpr uint8_t SERIAL_XT    = 0x00; // XT-compatible serial controller
	constexpr uint8_t SERIAL_16450 = 0x01; // 16450-compatible serial controller
	constexpr uint8_t SERIAL_16550 = 0x02; // 16550-compatible serial controller


	// Values of the class_api field for class_base = 0x07 (simple communications), class_sub = 0x01 (parallel port)
	constexpr uint8_t PARALLEL_SIMPLE        = 0x00; // Simple (output-only) parallel port
	constexpr uint8_t PARALLEL_BIDIRECTIONAL = 0x01; // Bidirectional parallel port
	constexpr uint8_t PARALLEL_ECP           = 0x02; // ECP 1.x compliant parallel port

	// Values for the class_sub field for class_base = 0x08 (generic system peripherals)
	constexpr uint8_t PIC                     = 0x00; // Peripheral interrupt controller
	constexpr uint8_t DMA                     = 0x01; // DMA controller
	constexpr uint8_t TIMER                   = 0x02; // Timers
	constexpr uint8_t RTC                     = 0x03; // Real time clock
	constexpr uint8_t GENERIC_HOT_PLUG        = 0x04; // Generic PCI hot-plug controller
	constexpr uint8_t SYSTEM_PERIPHERAL_OTHER = 0x80; // Other generic system peripheral

	// Values of the class_api field for class_base = 0x08 (generic system peripherals), class_sub = 0x00 (peripheral interrupt controller)
	constexpr uint8_t PIC_8259 = 0x00; // Generic 8259
	constexpr uint8_t PIC_ISA  = 0x01; // ISA pic
	constexpr uint8_t PIC_EISA = 0x02; // EISA pic

	// Values of the class_api field for class_base	= 0x08 (generic system peripherals), class_sub = 0x01 (dma controller)
	constexpr uint8_t DMA_8237 = 0x00; // Generic 8237
	constexpr uint8_t DMA_ISA  = 0x01; // ISA dma
	constexpr uint8_t DMA_EISA = 0x02; // EISA dma

	// Values of the class_api field for, class_base = 0x08 (generic system peripherals), class_sub = 0x02 (timer)
	constexpr uint8_t TIMER_8254 = 0x00; // Generic 8254
	constexpr uint8_t TIMER_ISA  = 0x01; // ISA timer
	constexpr uint8_t TIMER_EISA = 0x02; // EISA timers (2 timers)

	// Values of the class_api field for class_base	= 0x08 (generic system peripherals), class_sub = 0x03 (real time clock)
	constexpr uint8_t RTC_GENERIC = 0x00; // Generic real time clock
	constexpr uint8_t RTC_ISA     = 0x01; // ISA real time clock

	// Values for the class_sub field for class_base = 0x09 (input devices)
	constexpr uint8_t KEYBOARD    = 0x00; // Keyboard controller
	constexpr uint8_t PEN         = 0x01; // Pen
	constexpr uint8_t MOUSE       = 0x02; // Mouse controller
	constexpr uint8_t SCANNER     = 0x03; // Scanner controller
	constexpr uint8_t GAMEPORT    = 0x04; // Gameport controller
	constexpr uint8_t INPUT_OTHER = 0x80; // Other input controller

	// Values for the class_sub field for class_base = 0x0a (docking stations)
	constexpr uint8_t DOCKING_GENERIC = 0x00; // Generic docking station
	constexpr uint8_t DOCKING_OTHER   = 0x80; // Other docking stations

	// Values for the class_sub field for class_base = 0x0b (processor)
	constexpr uint8_t I386        = 0x00;
	constexpr uint8_t I486        = 0x01;
	constexpr uint8_t PENTIUM     = 0x02;
	constexpr uint8_t ALPHA       = 0x10;
	constexpr uint8_t POWERPC     = 0x20;
	constexpr uint8_t MIPS        = 0x30;
	constexpr uint8_t COPROCESSOR = 0x40;

	// Values for the class_sub field for class_base = 0x0c (serial bus controller)
	constexpr uint8_t FIREWIRE      = 0x00;
	constexpr uint8_t ACCESS        = 0x01;
	constexpr uint8_t SSA           = 0x02;
	constexpr uint8_t USB           = 0x03;
	constexpr uint8_t FIBRE_CHANNEL = 0x04;
	constexpr uint8_t SMBUS         = 0x05;
	constexpr uint8_t INFINIBAND    = 0x06;
	constexpr uint8_t IPMI          = 0x07;
	constexpr uint8_t SERCOS        = 0x08;
	constexpr uint8_t CANBUS        = 0x09;

	// Values of the class_api field for class_base = 0x0c (serial bus controller), class_sub = 0x03 (Universal Serial Bus)
	constexpr uint8_t USB_UHCI = 0x00; // Universal Host Controller Interface
	constexpr uint8_t USB_OHCI = 0x10; // Open Host Controller Interface
	constexpr uint8_t USB_EHCI = 0x20; // Enhanced Host Controller Interface

	// Values for the class_sub field for class_base = 0x0d (wireless controller)
	constexpr uint8_t WIRELESS_IRDA        = 0x00;
	constexpr uint8_t WIRELESS_CONSUMER_IR = 0x01;
	constexpr uint8_t WIRELESS_RF          = 0x02;
	constexpr uint8_t WIRELESS_BLUETOOTH   = 0x03;
	constexpr uint8_t WIRELESS_BROADBAND   = 0x04;
	constexpr uint8_t WIRELESS_80211A      = 0x10;
	constexpr uint8_t WIRELESS_80211B      = 0x20;
	constexpr uint8_t WIRELESS_OTHER       = 0x80;

	// Masks for command register bits
	constexpr uint16_t COMMAND_IO           = 0x001; // 1/0 I/O space en/disabled
	constexpr uint16_t COMMAND_MEMORY       = 0x002; // 1/0 memory space en/disabled
	constexpr uint16_t COMMAND_MASTER       = 0x004; // 1/0 PCI master en/disabled
	constexpr uint16_t COMMAND_SPECIAL      = 0x008; // 1/0 PCI special cycles en/disabled
	constexpr uint16_t COMMAND_MWI          = 0x010; // 1/0 memory write & invalidate en/disabled
	constexpr uint16_t COMMAND_VGA_SNOOP    = 0x020; // 1/0 VGA pallette snoop en/disabled
	constexpr uint16_t COMMAND_PARITY       = 0x040; // 1/0 parity check en/disabled
	constexpr uint16_t COMMAND_ADDRESS_STEP = 0x080; // 1/0 address stepping en/disabled
	constexpr uint16_t COMMAND_SERR         = 0x100; // 1/0 SERR# en/disabled
	constexpr uint16_t COMMAND_FASTBACK     = 0x200; // 1/0 fast back-to-back en/disabled
	constexpr uint16_t COMMAND_INT_DISABLE  = 0x400; // 1/0 interrupt generation dis/enabled

	// Masks for status register bits
	constexpr uint16_t STATUS_CAPABILITIES           = 0x0010; // Capabilities list
	constexpr uint16_t STATUS_66_MHZ_CAPABLE         = 0x0020; // 66 MHz capable
	constexpr uint16_t STATUS_UDF_SUPPORTED          = 0x0040; // User-definable-features (UDF) supported
	constexpr uint16_t STATUS_FASTBACK               = 0x0080; // Fast back-to-back capable
	constexpr uint16_t STATUS_PARITY_SIGNALLED       = 0x0100; // Parity error signalled
	constexpr uint16_t STATUS_DEVSEL                 = 0x0600; // Devsel timing (see below)
	constexpr uint16_t STATUS_TARGET_ABORT_SIGNALLED = 0x0800; // Signaled a target abort
	constexpr uint16_t STATUS_TARGET_ABORT_RECEIVED  = 0x1000; // Received a target abort
	constexpr uint16_t STATUS_MASTER_ABORT_RECEIVED  = 0x2000; // Received a master abort
	constexpr uint16_t STATUS_SERR_SIGNALLED         = 0x4000; // Signalled SERR#
	constexpr uint16_t STATUS_PARITY_ERROR_DETECTED  = 0x8000; // Parity error detected

	// Masks for devsel field in status register
	constexpr uint16_t STATUS_DEVSEL_FAST   = 0x0000; // Fast
	constexpr uint16_t STATUS_DEVSEL_MEDIUM = 0x0200; // Medium
	constexpr uint16_t STATUS_DEVSEL_SLOW   = 0x0400; // Slow

	// Header type register masks
	constexpr uint8_t HEADER_TYPE_MASK = 0x7f; // Header type field
	constexpr uint8_t MULTIFUNCTION    = 0x80; // Multifunction device flag

	// PCI header types
	constexpr uint8_t HEADER_TYPE_GENERIC           = 0x00;
	constexpr uint8_t HEADER_TYPE_PCI_TO_PCI_BRIDGE = 0x01;
	constexpr uint8_t HEADER_TYPE_CARDBUS           = 0x02;

	// Masks for built in self test (BIST) register bits
	constexpr uint8_t BIST_CODE    = 0x0f; // Self-test completion code, 0 = success
	constexpr uint8_t BIST_START   = 0x40; // 1 = start self-test
	constexpr uint8_t BIST_CAPABLE = 0x80; // 1 = self-test capable

	// Masks for flags in the various base address registers
	constexpr uint8_t ADDRESS_SPACE    = 0x01; // 0 = memory space, 1 = I/O space
	constexpr uint8_t REGISTER_START   = 0x10;
	constexpr uint8_t REGISTER_END     = 0x24;
	constexpr uint8_t REGISTER_PPB_END = 0x18;
	constexpr uint8_t REGISTER_PCB_END = 0x14;

	// Masks for flags in memory space base address registers
	constexpr uint8_t ADDRESS_TYPE_32      = 0x00; // Locate anywhere in 32-bit space
	constexpr uint8_t ADDRESS_TYPE_32_LOW  = 0x02; // Locate below 1 MB
	constexpr uint8_t ADDRESS_TYPE_64      = 0x04; // Locate anywhere in 64-bit space
	constexpr uint8_t ADDRESS_TYPE         = 0x06; // Type (see below)
	constexpr uint8_t ADDRESS_PREFETCHABLE = 0x08; // 1 if prefetchable (see PCI spec)

	constexpr uint32_t ADDRESS_MEMORY_32_MASK = 0xfffffff0; // Mask to get 32-bit memory space base address

	// Masks for flags in I/O space base address registers
	constexpr uint32_t ADDRESS_IO_MASK = 0xfffffffc; // Mask to get I/O space base address

	// Masks for flags in expansion rom base address registers
	constexpr uint32_t ROM_ENABLE       = 0x00000001; // 1 = expansion rom decode enabled
	constexpr uint32_t ROM_ADDRESS_MASK = 0xfffff800; // mask to get expansion rom addr

	// PCI interrupt pin values
	constexpr uint8_t PIN_MASK = 0x07;
	constexpr uint8_t PIN_NONE = 0x00;
	constexpr uint8_t PIN_A    = 0x01;
	constexpr uint8_t PIN_B    = 0x02;
	constexpr uint8_t PIN_C    = 0x03;
	constexpr uint8_t PIN_D    = 0x04;
	constexpr uint8_t PIN_MAX  = 0x04;

	// PCI capability codes
	constexpr uint8_t CAP_ID_RESERVED     = 0x00;
	constexpr uint8_t CAP_ID_PM           = 0x01; // Power management
	constexpr uint8_t CAP_ID_AGP          = 0x02; // AGP
	constexpr uint8_t CAP_ID_VPD          = 0x03; // Vital product data
	constexpr uint8_t CAP_ID_SLOTID       = 0x04; // Slot ID
	constexpr uint8_t CAP_ID_MSI          = 0x05; // Message signalled interrupt
	constexpr uint8_t CAP_ID_CHSWP        = 0x06; // Compact PCI HotSwap
	constexpr uint8_t CAP_ID_PCIX         = 0x07; // PCI-X
	constexpr uint8_t CAP_ID_LDT          = 0x08;
	constexpr uint8_t CAP_ID_VENDSPEC     = 0x09;
	constexpr uint8_t CAP_ID_DEBUGPORT    = 0x0a;
	constexpr uint8_t CAP_ID_CPCI_RSRCCTL = 0x0b;
	constexpr uint8_t CAP_ID_HOTPLUG      = 0x0c;
	constexpr uint8_t CAP_ID_SUBVENDOR    = 0x0d;
	constexpr uint8_t CAP_ID_AGP8X        = 0x0e;
	constexpr uint8_t CAP_ID_SECURE_DEV   = 0x0f;
	constexpr uint8_t CAP_ID_PCIE         = 0x10; // PCIe (PCI express)
	constexpr uint8_t CAP_ID_MSIX         = 0x11; // MSI-X
	constexpr uint8_t CAP_ID_SATA         = 0x12; // Serial ATA Capability
	constexpr uint8_t CAP_ID_PCIAF        = 0x13; // PCI Advanced Features

	// Power Management Control Status Register settings
	constexpr uint8_t  PM_MASK     = 0x03;
	constexpr uint8_t  PM_CTRL     = 0x02;
	constexpr uint16_t PM_D1SUPP   = 0x0200;
	constexpr uint16_t PM_D2SUPP   = 0x0400;
	constexpr uint8_t  PM_STATUS   = 0x04;
	constexpr uint8_t  PM_STATE_D0 = 0x00;
	constexpr uint8_t  PM_STATE_D1 = 0x01;
	constexpr uint8_t  PM_STATE_D2 = 0x02;
	constexpr uint8_t  PM_STATE_D3 = 0x03;

	constexpr uint16_t INVALID_VENDOR = 0xffff;
	constexpr uint8_t MSI_CONTROL_MME_MASK = 7 << 4;
	constexpr uint16_t MSI_CONTROL_VECTOR_MASKING = 1 << 8;
	#define PCI_MSI_CONTROL_SET_MME(x) ((x & 0x7) << 4)
}
