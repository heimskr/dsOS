// Some code is from https://github.com/imgits/ShellcodeOS/blob/master/OS/pci/pci.cpp

#include "hardware/AHCI.h"
#include "hardware/PCI.h"
#include "hardware/PCIIDs.h"
#include "hardware/Ports.h"
#include "lib/printf.h"
#include "memory/Memory.h"

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

	uint8_t readByte(const BSF &bsf, uint32_t offset) {
		return readByte(bsf.bus, bsf.slot, bsf.function, offset);
	}

	uint16_t readWord(const BSF &bsf, uint32_t offset) {
		return readWord(bsf.bus, bsf.slot, bsf.function, offset);
	}

	uint32_t readInt(const BSF &bsf, uint32_t offset) {
		return readInt(bsf.bus, bsf.slot, bsf.function, offset);
	}

	void writeByte(const BSF &bsf, uint32_t offset, uint8_t val) {
		writeByte(bsf.bus, bsf.slot, bsf.function, offset, val);
	}

	void writeWord(const BSF &bsf, uint32_t offset, uint16_t val) {
		writeWord(bsf.bus, bsf.slot, bsf.function, offset, val);
	}

	void writeInt(const BSF &bsf, uint32_t offset, uint32_t val) {
		writeInt(bsf.bus, bsf.slot, bsf.function, offset, val);
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

	std::vector<BSF> getDevices(uint32_t base_class, uint32_t subclass) {
		std::vector<BSF> out;
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t slot = 0; slot < 32; ++slot)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, slot, function);
					if (vendor == INVALID_VENDOR)
						break;
					if (getBaseClass(bus, slot, function) == base_class && getSubclass(bus, slot, function) == subclass)
						out.push_back({bus, slot, function});
				}
		return out;
	}

	HeaderNative readNativeHeader(const BSF &bsf) {
		HeaderNative native;
		const uint32_t bar5 = readInt(bsf, BAR5);
		for (uint8_t i = 0; i < 64; i += 16) {
			((uint32_t *) &native)[i]     = readInt(bsf, i);
			((uint32_t *) &native)[i + 1] = readInt(bsf, i + 4);
			((uint32_t *) &native)[i + 2] = readInt(bsf, i + 8);
			((uint32_t *) &native)[i + 3] = readInt(bsf, i + 12);
		}
		native.bar5 = bar5;
		return native;
	}

	Device * initDevice(const BSF &bsf) {
		const HeaderNative native = readNativeHeader(bsf);
		return native.vendorID == INVALID_VENDOR? nullptr : new Device(bsf, native);
	}

	bool findAHCIController() {
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t slot = 0; slot < 32; ++slot)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, slot, function);

					if (vendor == INVALID_VENDOR)
						break;

					const uint32_t baseclass = getBaseClass(bus, slot, function);
					const uint32_t subclass  = getSubclass(bus, slot, function);

					if (baseclass == 1 && subclass == 6) {
						AHCI::controller = initDevice({bus, slot, function});
						AHCI::abar = (AHCI::HBAMemory *) (uint64_t) (AHCI::controller->nativeHeader.bar5 & ~0xfff);
						return true;
					}
				}
		return false;
	}

	size_t printDevices() {
		size_t device_count = 0;
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t slot = 0; slot < 32; ++slot)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, slot, function);

					if (vendor == INVALID_VENDOR)
						break;

					const uint32_t device    = getDeviceID(bus, slot, function);
					const uint32_t baseclass = getBaseClass(bus, slot, function);
					const uint32_t subclass  = getSubclass(bus, slot, function);

					if (ID::IDSet *pci_ids = ID::getDeviceIDs(vendor, device, 0, 0))
						printf("%lu %x:%x:%x Device: %s (vendor: %x, device: %x, class: %x, subclass: %x)\n",
							device_count++, bus, slot, function, pci_ids->device_name, vendor, device, baseclass, subclass);
					else
						printf("%lu %x:%x:%x Vendor: %x, device: %x, class: %x, subclass: %x\n",
							device_count++, bus, slot, function, vendor, device, baseclass, subclass);

					uint32_t header_type = getHeaderType(bus, slot, function);
					if ((header_type & 0x80) == 0)
						break;
				}
		return device_count;
	}
}
