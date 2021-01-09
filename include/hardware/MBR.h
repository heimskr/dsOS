#pragma once

#include <cstdint>

namespace DsOS {
	struct CHS {
		uint8_t cylinders;
		uint8_t heads;
		uint8_t sectors;

		CHS(uint8_t cylinders_, uint8_t heads_, uint8_t sectors_):
			cylinders(cylinders_), heads(heads_), sectors(sectors_) {}

		uint32_t toLBA(uint8_t heads_per_cylinder, uint8_t sectors_per_track) {
			return (cylinders * heads_per_cylinder + heads) * sectors_per_track + sectors - 1;
		}

		static CHS fromLBA(uint32_t lba, uint8_t heads_per_cylinder, uint8_t sectors_per_track) {
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
	} __attribute__((packed));

	struct MBR {
		uint8_t bootstrap[440] = {0};
		uint8_t diskID[4] = {0};
		uint8_t reserved[2] = {0};
		MBREntry firstEntry;
		MBREntry secondEntry;
		MBREntry thirdEntry;
		MBREntry fourthEntry;
		uint8_t signature[2] = {0x55, 0xaa};
	};
}
