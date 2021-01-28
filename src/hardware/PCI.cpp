// Some code is from https://github.com/imgits/ShellcodeOS/blob/master/OS/pci/pci.cpp

#include "hardware/AHCI.h"
#include "hardware/PCI.h"
#include "hardware/PCIIDs.h"
#include "hardware/Ports.h"
#include "hardware/Serial.h"
#include "lib/printf.h"
#include "memory/Memory.h"
#include "DsUtil.h"
#include "Terminal.h"

namespace DsOS::PCI {
	using namespace DsOS::Ports;

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

	uint8_t readByte(const BDF &bdf, uint32_t offset) {
		return readByte(bdf.bus, bdf.device, bdf.function, offset);
	}

	uint16_t readWord(const BDF &bdf, uint32_t offset) {
		return readWord(bdf.bus, bdf.device, bdf.function, offset);
	}

	uint32_t readInt(const BDF &bdf, uint32_t offset) {
		return readInt(bdf.bus, bdf.device, bdf.function, offset);
	}

	void writeByte(const BDF &bdf, uint32_t offset, uint8_t val) {
		writeByte(bdf.bus, bdf.device, bdf.function, offset, val);
	}

	void writeWord(const BDF &bdf, uint32_t offset, uint16_t val) {
		writeWord(bdf.bus, bdf.device, bdf.function, offset, val);
	}

	void writeInt(const BDF &bdf, uint32_t offset, uint32_t val) {
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
		const uint32_t bar5 = readInt(bdf, BAR5);
		for (uint8_t i = 0; i < 64; i += 16) {
			((uint32_t *) &native)[i]     = readInt(bdf, i);
			((uint32_t *) &native)[i + 1] = readInt(bdf, i + 4);
			((uint32_t *) &native)[i + 2] = readInt(bdf, i + 8);
			((uint32_t *) &native)[i + 3] = readInt(bdf, i + 12);
		}
		native.bar5 = bar5;
	}

	Device * initDevice(const BDF &bdf) {
		const HeaderNative native = readNativeHeader(bdf);
		return native.vendorID == INVALID_VENDOR? nullptr : new Device(bdf, native);
	}

	void scan() {
		// HeaderNative header;
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t device = 0; device < 32; ++device)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, device, function);

					// printf("[%x:%x:%x]: %x\n", bus, device, function, vendor);

					if (vendor == INVALID_VENDOR)
						continue;

					const uint32_t baseclass = getBaseClass(bus, device, function);
					const uint32_t subclass  = getSubclass(bus, device, function);
					const uint32_t interface = getProgIF(bus, device, function);

					if (baseclass == 1 && subclass == 6) {
						AHCI::controller = initDevice({bus, device, function});
						AHCI::abar = (AHCI::HBAMemory *) (uintptr_t) (AHCI::controller->nativeHeader.bar5 & ~0xfff);
						printf("Found AHCI controller at %x:%x:%x.\n", bus, device, function);
					} else if (baseclass == 12 && subclass == 3 && interface == 0) {
						// Bizarrely, this prevents a general protection fault caused by a return to an invalid address.
						// Util::getReturnAddress();
						HeaderNative header = readNativeHeader({bus, device, function});
						// readNativeHeader({bus, device, function}, header);
						printf("Found UHCI controller at %x:%x:%x [0=0x%lx, 1=0x%lx, 2=0x%lx, 3=0x%lx, 4=0x%lx, 5=0x%lx]\n", bus, device, function, header.bar0, header.bar1, header.bar2, header.bar3, header.bar4, header.bar5);
						// printf("Found UHCI controller at %x:%x:%x [0=0x%lx, 1=0x%lx, 2=0x%lx, 3=0x%lx, 4=0x%lx, 5=0x%lx]\n", bus, device, function);
					} else {
						printf("%x,%x,%x at %x:%x:%x\n", baseclass, subclass, interface, bus, device, function);
					}
				}
		// This is also needed to prevent a general protection fault.
		// Util::getReturnAddress();
		printf("\n");
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

					const uint32_t deviceID  = getDeviceID(bus, device, function);
					const uint32_t baseclass = getBaseClass(bus, device, function);
					const uint32_t subclass  = getSubclass(bus, device, function);
					const uint32_t interface = getProgIF(bus, device, function);

					if (ID::IDSet *pci_ids = ID::getDeviceIDs(vendor, deviceID, 0, 0))
						printf("%lu %x:%x:%x Device: %s (%x %x %x) ",
							device_count++, bus, device, function, pci_ids->device_name, baseclass, subclass, interface);
					else
						printf("%lu %x:%x:%x Vendor: %x, device: %x, class: %x, subclass: %x ",
							device_count++, bus, device, function, vendor, deviceID, baseclass, subclass);
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
