#pragma once

#include <cstdint>
#include <cstring>

namespace Thorn {
	struct CHS {
		uint8_t cylinders = 0;
		uint8_t heads = 0;
		uint8_t sectors = 0;

		CHS() = default;
		CHS(uint8_t cylinders_, uint8_t heads_, uint8_t sectors_):
			cylinders(cylinders_), heads(heads_), sectors(sectors_) {}

		inline uint32_t toLBA(uint8_t heads_per_cylinder = 16, uint8_t sectors_per_track = 63) const {
			return (cylinders * heads_per_cylinder + heads) * sectors_per_track + sectors - 1;
		}

		static CHS fromLBA(uint32_t lba, uint8_t heads_per_cylinder = 16, uint8_t sectors_per_track = 63) {
			return {
				(uint8_t) (lba / (heads_per_cylinder * sectors_per_track)),
				(uint8_t) ((lba / sectors_per_track) % heads_per_cylinder),
				(uint8_t) ((lba % heads_per_cylinder) + 1)
			};
		}
	} __attribute__((packed));

	struct MBREntry {
		uint8_t attributes = 0;
		CHS startCHS;
		uint8_t type = 0;
		CHS lastSectorCHS;
		uint32_t startLBA = 0;
		uint32_t sectors = 0;

		MBREntry() = default;
		MBREntry(uint8_t attributes_, uint8_t type_, uint32_t startLBA_, uint32_t sectors_):
			attributes(attributes_), startCHS(CHS::fromLBA(startLBA_)), type(type_),
			lastSectorCHS(CHS::fromLBA(startLBA_ + sectors_)), startLBA(startLBA_), sectors(sectors_) {}
	} __attribute__((packed));

	struct MBR {
		uint8_t bootstrap[440] = {};
		uint8_t diskID[4] = {};
		uint8_t reserved[2] = {};
		MBREntry firstEntry;
		MBREntry secondEntry;
		MBREntry thirdEntry;
		MBREntry fourthEntry;
		uint8_t signature[2] = {0x55, 0xaa};
		MBR() = default;
		MBR(char *buffer) { // Buffer is expected to be 512 bytes long
			size_t offset = 0;
			memcpy(bootstrap,    buffer,                                   sizeof(bootstrap));
			memcpy(diskID,       buffer + (offset += sizeof(bootstrap)),   sizeof(diskID));
			memcpy(reserved,     buffer + (offset += sizeof(diskID)),      sizeof(reserved));
			memcpy(&firstEntry,  buffer + (offset += sizeof(reserved)),    sizeof(firstEntry));
			memcpy(&secondEntry, buffer + (offset += sizeof(firstEntry)),  sizeof(secondEntry));
			memcpy(&thirdEntry,  buffer + (offset += sizeof(secondEntry)), sizeof(thirdEntry));
			memcpy(&fourthEntry, buffer + (offset += sizeof(thirdEntry)),  sizeof(fourthEntry));
			memcpy(signature,    buffer + (offset += sizeof(fourthEntry)), sizeof(signature));
		}
		bool indicatesGPT() const;
	} __attribute__((packed));
}
