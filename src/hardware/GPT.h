#pragma

#include <cstdint>

namespace DsOS {
	struct GPTHeader {
		uint64_t signature;
		uint32_t revision;
		uint32_t headerSize;
		uint32_t crc32;
		uint32_t reserved;
		uint64_t currentLBA;
		uint64_t otherLBA;
		uint64_t firstLBA; // First usable LBA for partitions
		uint64_t lastLBA;  // Last usable LBA for partitions
		uint8_t guid[16];
		uint64_t startLBA; // Starting LBA of partition entries
		uint32_t partitionCount;
		uint32_t partitionEntrySize;
		uint32_t partitionsCRC32;
	};
}
