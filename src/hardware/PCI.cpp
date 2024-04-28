// Some code is from https://github.com/imgits/ShellcodeOS/blob/master/OS/pci/pci.cpp
// Some code is from https://github.com/fido2020/Lemon-OS/blob/master/Kernel/include/arch/x86_64/pci.h

#include "arch/x86_64/APIC.h"
#include "arch/x86_64/CPU.h"
#include "arch/x86_64/Interrupts.h"
#include "hardware/AHCI.h"
#include "hardware/PCI.h"
#include "hardware/PCIIDs.h"
#include "hardware/Ports.h"
#include "hardware/Serial.h"
#include "hardware/UHCI.h"
#include "lib/printf.h"
#include "memory/Memory.h"
#include "ThornUtil.h"
#include "Terminal.h"

namespace Thorn::PCI {
	using namespace Thorn::Ports;

	uint8_t readByte(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc));
		return inb(0xcfc + (offset & 0x03));
	}

	uint16_t readWord(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc));
		return inw(0xcfc + (offset & 0x02));
	}

	uint32_t readInt(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc));
		return inl(0xcfc);
	}

	void writeByte(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset, uint8_t val) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc));
		outb(0xcfc + (offset & 0x03),val);
	}

	void writeWord(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset, uint16_t val) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc));
		outw(0xcfc + (offset & 0x02), val);
	}

	void writeInt(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset, uint32_t val) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xfc));
		outl(0xcfc, val);
	}

	uint8_t readByte(const volatile BDF &bdf, uint32_t offset) {
		return readByte(bdf.bus, bdf.device, bdf.function, offset);
	}

	uint16_t readWord(const volatile BDF &bdf, uint32_t offset) {
		return readWord(bdf.bus, bdf.device, bdf.function, offset);
	}

	uint32_t readInt(const volatile BDF &bdf, uint32_t offset) {
		return readInt(bdf.bus, bdf.device, bdf.function, offset);
	}

	void writeByte(const volatile BDF &bdf, uint32_t offset, uint8_t val) {
		writeByte(bdf.bus, bdf.device, bdf.function, offset, val);
	}

	void writeWord(const volatile BDF &bdf, uint32_t offset, uint16_t val) {
		writeWord(bdf.bus, bdf.device, bdf.function, offset, val);
	}

	void writeInt(const volatile BDF &bdf, uint32_t offset, uint32_t val) {
		writeInt(bdf.bus, bdf.device, bdf.function, offset, val);
	}

	uint16_t getVendorID(uint32_t bus, uint32_t device, uint32_t function) {
		return readWord(bus, device, function, 0x00);
	}

	uint16_t getDeviceID(uint32_t bus, uint32_t device, uint32_t function) {
		return readWord(bus, device, function, 0x02);
	}

	uint16_t getCommand(uint32_t bus, uint32_t device, uint32_t function) {
		return readWord(bus, device, function, 0x04);
	}

	uint16_t getStatus(uint32_t bus, uint32_t device, uint32_t function) {
		return readWord(bus, device, function, STATUS);
	}

	uint8_t getRevisionID(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, REVISION);
	}

	uint8_t getProgIF(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, CLASS_API);
	}

	uint8_t getBaseClass(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, CLASS_BASE);
	}

	uint8_t getSubclass(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, CLASS_SUB);
	}

	uint8_t getCacheLineSize(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, LINE_SIZE);
	}

	uint8_t getLatencyTimer(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, LATENCY);
	}

	uint8_t getHeaderType(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, HEADER_TYPE);
	}

	uint8_t getBIST(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, BIST);
	}

	std::vector<BDF> getDevices(uint32_t base_class, uint32_t subclass) {
		std::vector<BDF> out;
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t device = 0; device < 32; ++device)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, device, function);
					if (vendor == INVALID_VENDOR)
						break;
					if (getBaseClass(bus, device, function) == base_class && getSubclass(bus, device, function) == subclass)
						out.push_back({bus, device, function});
				}
		return out;
	}

	HeaderNative readNativeHeader(const BDF &bdf) {
		HeaderNative native;
		readNativeHeader(bdf, native);
		return native;
	}

	void readNativeHeader(const BDF &bdf, HeaderNative &native) {
		native.vendorID                = readWord(bdf, VENDOR_ID);
		native.deviceID                = readWord(bdf, DEVICE_ID);
		native.command                 = readWord(bdf, COMMAND);
		native.status                  = readWord(bdf, STATUS);
		native.revision                = readByte(bdf, REVISION);
		native.progif                  = readByte(bdf, CLASS_API);
		native.subclass                = readByte(bdf, CLASS_SUB);
		native.classCode               = readByte(bdf, CLASS_BASE);
		native.cacheLineSize           = readByte(bdf, LINE_SIZE);
		native.latencyTimer            = readByte(bdf, LATENCY);
		native.headerType              = readByte(bdf, HEADER_TYPE);
		native.bist                    = readByte(bdf, BIST);
		native.bar0                    = readInt(bdf, BAR0);
		native.bar1                    = readInt(bdf, BAR1);
		native.bar2                    = readInt(bdf, BAR2);
		native.bar3                    = readInt(bdf, BAR3);
		native.bar4                    = readInt(bdf, BAR4);
		native.bar5                    = readInt(bdf, BAR5);
		native.cardbusCISPointer       = readInt(bdf, CARDBUS_CIS);
		native.subsystemVendorID       = readWord(bdf, SUBSYSTEM_VENDOR_ID);
		native.subsystemID             = readWord(bdf, SUBSYSTEM_ID);
		native.expansionROMBaseAddress = readInt(bdf, ROM_BASE);
		native.capabilitiesPointer     = readByte(bdf, CAPABILITIES_PTR);
		native.interruptLine           = readByte(bdf, INTERRUPT_LINE);
		native.interruptPin            = readByte(bdf, INTERRUPT_PIN);
		native.minGrant                = readByte(bdf, MIN_GRANT);
		native.maxLatency              = readByte(bdf, MAX_LATENCY);
	}

	Device * initDevice(const BDF &bdf) {
		Device *device = new Device(bdf);
		device->init();
		return device;
	}

	Device::Device(const BDF &bdf_): bdf(bdf_) {}

	void Device::init() {
		if (readStatus() & STATUS_CAPABILITIES) {
			uint8_t ptr = readWord(CAPABILITIES_PTR) & 0xfc;
			uint16_t cap = readInt(ptr);
			do {
				if ((cap & 0xff) == CAP_ID_MSI) {
					msiPointer = ptr;
					msiCapable = true;
					msiCapability.register0 = readInt(ptr);
					msiCapability.register1 = readInt(ptr + sizeof(uint32_t));
					msiCapability.register2 = readInt(ptr + sizeof(uint32_t) * 2);
					if (msiCapability.msiControl & MSI_CONTROL_64)
						msiCapability.data64 = readInt(ptr + sizeof(uint32_t) * 3);
				}

				ptr = cap >> 8;
				cap = readWord(ptr);
				capabilities.push_back(cap & 0xff);
			} while (cap >> 8);
		}
	}

	uint16_t Device::readStatus() {
		return readWord(STATUS);
	}

	uintptr_t Device::getBAR(uint8_t index) {
		uintptr_t bar = readInt(BAR0 + index * sizeof(uint32_t));
		if (!(bar & 1) && (bar & 4) && index < 5)
			bar |= static_cast<uintptr_t>(readInt(BAR0 + (bar + 1) * sizeof(uint32_t))) << 32;
		return bar & ((bar & 1)? 0xfffffffffffffffc : 0xfffffffffffffff0);
	}

	uint32_t Device::rawBAR(uint8_t index) {
		return readInt(BAR0 + index * sizeof(uint32_t));
	}

	uint16_t Device::getCommand() {
		return readWord(COMMAND);
	}

	void Device::setCommand(uint16_t command) {
		writeWord(COMMAND, command);
	}

	uint8_t Device::getInterruptLine() {
		return readByte(INTERRUPT_LINE);
	}

	uint8_t Device::getInterruptPin() {
		return readByte(INTERRUPT_PIN);
	}

	uint8_t Device::allocateVector(Vector vector) {
		return allocateVector(static_cast<uint8_t>(vector));
	}

	uint8_t Device::allocateVector(uint8_t vector) {
		if (vector & static_cast<uint8_t>(Vector::MSI)) {
			if (!msiCapable) {
				printf("[PCI::Device::allocateVector] Device isn't MSI capable.\n");
			} else {
				uint8_t interrupt = x86_64::IDT::reserveUnusedInterrupt();
				if (interrupt == 0xff) {
					printf("[PCI::Device::allocateVector] Device isn't MSI capable.\n");
					return interrupt;
				}

				msiCapability.msiControl = (msiCapability.msiControl &
					~(MSI_CONTROL_MME_MASK | MSI_CONTROL_VECTOR_MASKING)) | PCI_MSI_CONTROL_SET_MME(0);
				msiCapability.msiControl |= 1; // Enable MSIs

				msiCapability.setData((interrupt & 0xff) | x86_64::APIC::ICR_MESSAGE_TYPE_FIXED);
				// msiCapability.setAddress(x86_64::getCPULocal()->id);

				if (msiCapability.msiControl & MSI_CONTROL_64)
					writeInt(msiPointer + sizeof(uint32_t) * 3, msiCapability.register3);
				writeInt(msiPointer + sizeof(uint32_t) * 2, msiCapability.register2);
				writeInt(msiPointer + sizeof(uint32_t), msiCapability.register1);
				writeInt(msiPointer + sizeof(uint16_t), msiCapability.msiControl);

				return interrupt;
			}
		}

		if (vector & static_cast<uint8_t>(Vector::Legacy)) {
			const uint8_t irq = getInterruptLine();
			if (irq == 0xff) {
				printf("[PCI::Device::allocateVector] Legacy interrupts aren't supported by device.\n");
				return 0xff;
			} else {
				// TODO
				// APIC::IO::MapLegacyIRQ(irq);
			}

			return 32 | irq;
		}

		printf("[PCI::Device::allocateVector] Could not allocate interrupt (type %d)!", static_cast<int>(vector));
		return 0xff;
	}


	uint8_t  Device::readByte(uint32_t offset) {
		return ::Thorn::PCI::readByte(bdf, offset);
	}

	uint16_t Device::readWord(uint32_t offset) {
		return ::Thorn::PCI::readWord(bdf, offset);
	}

	uint32_t Device::readInt(uint32_t offset) {
		return ::Thorn::PCI::readInt(bdf, offset);
	}

	void Device::writeByte(uint32_t offset, uint8_t val) {
		return ::Thorn::PCI::writeByte(bdf, offset, val);
	}

	void Device::writeWord(uint32_t offset, uint16_t val) {
		return ::Thorn::PCI::writeWord(bdf, offset, val);
	}

	void Device::writeInt(uint32_t offset, uint32_t val) {
		return ::Thorn::PCI::writeInt(bdf, offset, val);
	}

	void Device::setBusMastering(bool enable) {
		if (enable)
			writeWord(COMMAND, readWord(COMMAND) | COMMAND_MASTER);
		else
			writeWord(COMMAND, readWord(COMMAND) & ~COMMAND_MASTER);
	}

	void Device::setInterrupts(bool enable) {
		if (enable)
			writeWord(COMMAND, readWord(COMMAND) & ~COMMAND_INT_DISABLE);
		else
			writeWord(COMMAND, readWord(COMMAND) | COMMAND_INT_DISABLE);
	}

	void Device::setMemorySpace(bool enable) {
		if (enable)
			writeWord(COMMAND, readWord(COMMAND) | COMMAND_MEMORY);
		else
			writeWord(COMMAND, readWord(COMMAND) & ~COMMAND_MEMORY);
	}

	void Device::setIOSpace(bool enable) {
		if (enable)
			writeWord(COMMAND, readWord(COMMAND) | COMMAND_IO);
		else
			writeWord(COMMAND, readWord(COMMAND) & ~COMMAND_IO);
	}

	void scan() {
		// HeaderNative header;
		printf("Scanning.\n");
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t device = 0; device < 32; ++device)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, device, function);

					// printf("[%x:%x:%x]: %x\n", bus, device, function, vendor);

					if (vendor == INVALID_VENDOR || vendor == 0)
						continue;

					const uint32_t baseclass = getBaseClass(bus, device, function);
					const uint32_t subclass  = getSubclass(bus, device, function);
					const uint32_t interface = getProgIF(bus, device, function);

					if (baseclass == 1 && subclass == 6) {
						AHCI::controllers.push_back(AHCI::Controller(initDevice({bus, device, function})));
						printf("Found AHCI controller at %x:%x:%x.\n", bus, device, function);
					} else if (baseclass == 12 && subclass == 3 && interface == 0) {
						HeaderNative header = readNativeHeader({bus, device, function});
						printf("Found UHCI controller at %x:%x:%x [0=0x%llx, 1=0x%llx, 2=0x%llx, 3=0x%llx, 4=0x%llx, 5=0x%llx]\n", bus, device, function, header.bar0, header.bar1, header.bar2, header.bar3, header.bar4, header.bar5);
						if (UHCI::controllers)
							UHCI::controllers->push_back(UHCI::Controller(initDevice({bus, device, function})));
					} else {
						printf("%x,%x,%x at %x:%x:%x\n", baseclass, subclass, interface, bus, device, function);
					}
				}
	}

	size_t printDevices() {
		size_t device_count = 0;
		bool white = true;
		Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::Cyan, Terminal::VGAColor::Black);
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t device = 0; device < 32; ++device)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, device, function);

					if (vendor == INVALID_VENDOR)
						continue;

					const uint32_t device_id  = getDeviceID(bus, device, function);
					const uint32_t baseclass = getBaseClass(bus, device, function);
					const uint32_t subclass  = getSubclass(bus, device, function);
					const uint32_t interface = getProgIF(bus, device, function);

					if (ID::IDSet *pci_ids = ID::getDeviceIDs(vendor, device_id, 0, 0))
						printf("%lu %x:%x:%x Device: %s (%x %x %x) ",
							device_count++, bus, device, function, pci_ids->deviceName, baseclass, subclass, interface);
					else
						printf("%lu %x:%x:%x Vendor: %x, device: %x, class: %x, subclass: %x ",
							device_count++, bus, device, function, vendor, device_id, baseclass, subclass);

					if (Serial::ready)
						Serial::write('\n');

					if (white)
						Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::Magenta, Terminal::VGAColor::Black);
					else
						Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::Cyan, Terminal::VGAColor::Black);

					white = !white;
				}
		Terminal::color = Terminal::vgaEntryColor(Terminal::VGAColor::LightGray, Terminal::VGAColor::Black);
		printf("\n");
		return device_count;
	}
}
