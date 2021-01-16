// Some code is from https://github.com/imgits/ShellcodeOS/blob/master/OS/pci/pci.cpp

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

	uint8_t getSubClass(uint32_t bus, uint32_t device, uint32_t function) {
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
					if (getBaseClass(bus, slot, function) == base_class && getSubClass(bus, slot, function) == subclass)
						out.push_back({bus, slot, function});
				}
		return out;
	}

	HeaderNative readNativeHeader(const BSF &bsf) {
		HeaderNative native;
		for (uint8_t i = 0; i < 64; i += 16) {
			((uint32_t *) &native)[i] = readInt(bsf.bus, bsf.slot, bsf.function, i);
			((uint32_t *) &native)[i + 1] = readInt(bsf.bus, bsf.slot, bsf.function, i + 4);
			((uint32_t *) &native)[i + 2] = readInt(bsf.bus, bsf.slot, bsf.function, i + 8);
			((uint32_t *) &native)[i + 3] = readInt(bsf.bus, bsf.slot, bsf.function, i + 12);
		}
		return native;
	}

	Device * initDevice(const BSF &bsf) {
		HeaderNative native = readNativeHeader(bsf);
		if (native.vendorID == INVALID_VENDOR)
			return nullptr;
		Device *device = new Device(0, bsf, {});
		printf("Header type: %d\n", native.headerType);
		printf("Class:subclass = %d:%d\n", native.classCode, native.subclass);
		printf("Vendor: 0x%x\n", native.vendorID);
		printf("command[%d], status[%d]\n", native.command, native.status);
		printf("Revision[%d], progif[%d], cacheLineSize[%d], latencyTimer[%d]\n", native.revision, native.progif, native.cacheLineSize, native.latencyTimer);
		printf("bist[%d], bar0[0x%x], bar1[0x%x], bar2[0x%x]\n", native.bist, native.bar0, native.bar1, native.bar2);
		printf("bar3[0x%x], bar4[0x%x], bar5[0x%x]\n", native.bar3, native.bar4, native.bar5);
		printf("cardbusCIS[0x%x], subsystemVendorID[0x%x], subsystemID[%d]\n", native.cardbusCISPointer, native.subsystemVendorID, native.subsystemID);
		printf("expansionROMBaseAddress[0x%x]\n", native.expansionROMBaseAddress);
		printf("capabilitiesPointer[%d]\n", native.capabilitiesPointer);
		printf("interruptLine[%d], interruptPIN[%d]\n", native.interruptLine, native.interruptPIN);
		printf("minGrant[%d], maxLatency[%d]\n", native.minGrant, native.maxLatency);
		return device;
	}

	size_t scanDevices() {
		size_t device_count = 0;
		for (uint32_t bus = 0; bus < 256; ++bus)
			for (uint32_t slot = 0; slot < 32; ++slot)
				for (uint32_t function = 0; function < 8; ++function) {
					const uint32_t vendor = getVendorID(bus, slot, function);

					if (vendor == INVALID_VENDOR)
						break;

					uint32_t device    = getDeviceID(bus, slot, function);
					uint32_t baseclass = getBaseClass(bus, slot, function);
					uint32_t subclass  = getSubClass(bus, slot, function);

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
