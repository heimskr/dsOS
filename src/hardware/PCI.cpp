// Some code is from https://github.com/imgits/ShellcodeOS/blob/master/OS/pci/pci.cpp

#include "hardware/PCI.h"
#include "hardware/PCIIDs.h"
#include "hardware/Ports.h"
#include "lib/printf.h"

namespace DsOS::PCI {
	using namespace DsOS::Ports;

	uint8_t readByte(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xfc));
		return inb(0xcfc + (offset & 0x03));
	}

	uint16_t readWord(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xfc));
		return inw(0xcfc + (offset & 0x02));
	}

	uint32_t readInt(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xfc));
		return inl(0xcfc);
	}

	void writeByte(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset, uint8_t val) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xfc));
		outb(0xcfc + (offset & 0x03),val);
	}

	void writeWord(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset, uint16_t val) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xfc));
		outw(0xcfc + (offset & 0x02), val);
	}

	void writeInt(uint32_t bus, uint32_t slot, uint32_t function, uint32_t offset, uint32_t val) {
		outl(0xcf8, 0x80000000 | (bus << 16) | (slot << 11) | (function << 8) | (offset & 0xfc));
		outl(0xcfc, val);
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
		return readWord(bus, device, function, 0x06);
	}

	uint8_t getRevisionID(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, 0x08);
	}

	uint8_t getProgIF(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, 0x09);
	}

	uint8_t getBaseClass(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, 0x0A);
	}

	uint8_t getSubClass(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, 0x0B);
	}

	uint8_t getCacheLineSize(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, 0x0C);
	}

	uint8_t getLatencyTimer(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, 0x0D);
	}

	uint8_t getHeaderType(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, 0x0E);
	}

	uint8_t getBIST(uint32_t bus, uint32_t device, uint32_t function) {
		return readByte(bus, device, function, 0x0F);
	}

	std::vector<BSF> getDevices(uint32_t base_class, uint32_t subclass) {
		std::vector<BSF> out;
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t slot = 0; slot < 32; ++slot)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, slot, function);
					if (vendor == 0xffff)
						break;
					if (getBaseClass(bus, slot, function) == base_class && getSubClass(bus, slot, function) == subclass)
						out.push_back({bus, slot, function});
				}
		return out;
	}

	size_t scanDevices() {
		size_t device_count = 0;
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t slot = 0; slot < 32; ++slot)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, slot, function);

					if (vendor == 0xffff)
						break;

					uint32_t device    = getDeviceID(bus, slot, function);
					uint32_t baseclass = getBaseClass(bus, slot, function);
					uint32_t subclass  = getSubClass(bus, slot, function);

					if (ID::IDSet *pci_ids = ID::getDeviceIDs(vendor, device, 0, 0)) {
						printf("%lu %x:%x:%x Device: %s (class: %x, subclass: %x)\n",
							device_count++, bus, slot, function, pci_ids->device_name, baseclass, subclass);
					} else {
						printf("%lu %x:%x:%x Vendor: %x, device: %x, class: %x, subclass: %x\n",
							device_count++, bus, slot, function, vendor, device, baseclass, subclass);
					}

					uint32_t header_type = getHeaderType(bus, slot, function);
					if ((header_type & 0x80) == 0)
						break;
				}
		return device_count;
	}
}
