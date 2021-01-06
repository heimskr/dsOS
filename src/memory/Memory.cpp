#include <stdlib.h>

#include "memory/Memory.h"
#include "memory/memset.h"

DsOS::Memory *global_memory = nullptr;

namespace DsOS {
	Memory::Memory(char *start_, char *high_): start(start_), high(high_), end(start_) {
		start = (char *) realign((uintptr_t) start);
		global_memory = this;
	}

	Memory::Memory(): Memory((char *) 0, (char *) 0) {}

	uintptr_t Memory::realign(uintptr_t val) {
		size_t offset = (val + sizeof(BlockMeta)) % MEMORY_ALIGN;
		if (offset)
			val += MEMORY_ALIGN - offset;
		return val;
	}

	Memory::BlockMeta * Memory::findFreeBlock(BlockMeta * &last, size_t size) {
		BlockMeta *current = base;
		while (current && !(current->free && current->size >= size)) {
			last = current;
			current = current->next;
		}
		return current;
	}

	Memory::BlockMeta * Memory::requestSpace(BlockMeta *last, size_t size) {
		BlockMeta *block = (BlockMeta *) realign((uintptr_t) end);

		if (last)
			last->next = block;

		block->size = size;
		block->next = nullptr;
		block->free = 0;

		end = reinterpret_cast<char *>(block) + block->size + sizeof(BlockMeta) + 1;

		return block;
	}

	void * Memory::allocate(size_t size, size_t /* alignment */) {
		BlockMeta *block = nullptr;

		if (size <= 0)
			return nullptr;

		if (!base) {
			block = requestSpace(nullptr, size);
			if (!block)
				return nullptr;
			base = block;
		} else {
			BlockMeta *last = base;
			block = findFreeBlock(last, size);
			if (!block) {
				block = requestSpace(last, size);
				if (!block)
					return nullptr;
			} else {
				split(*block, size);
				block->free = 0;
			}
		}

		allocated += block->size + sizeof(BlockMeta);
		return block + 1;
	}

	void Memory::split(BlockMeta &block, size_t size) {
		if (block.size > size + sizeof(BlockMeta)) {
			// We have enough space to split the block, unless alignment takes up too much.
			BlockMeta *new_block = (BlockMeta *) realign((uintptr_t) &block + size + sizeof(BlockMeta) + 1);

			// After we realign, we need to make sure that the new block's new size isn't negative.

			if (block.next) {
				const int new_size = (char *) block.next - (char *) new_block - sizeof(BlockMeta);

				// Realigning the new block can make it too small, so we need to make sure the new block is big enough.
				if (new_size > 0) {
					new_block->size = new_size;
					new_block->next = block.next;
					new_block->free = 1;
					block.next = new_block;
					block.size = size;
				}
			} else {
				const int new_size = (char *) &block + block.size - (char *) new_block;

				if (new_size > 0) {
					new_block->size = new_size;
					new_block->free = 1;
					new_block->next = nullptr;
					block.size = size;
					block.next = new_block;
				}
			}
		}
	}

	Memory::BlockMeta * Memory::getBlock(void *ptr) {
		return (BlockMeta *) ptr - 1;
	}

	void Memory::free(void *ptr) {
		if (!ptr)
			return;

		BlockMeta *block_ptr = getBlock(ptr);
		block_ptr->free = 1;
		allocated -= block_ptr->size + sizeof(BlockMeta);
		merge();
	}

	int Memory::merge() {
		int count = 0;
		BlockMeta *current = base;
		while (current && current->next) {
			if (current->free && current->next->free) {
				current->size += sizeof(BlockMeta) + current->next->size;
				current->next = current->next->next;
				count++;
			} else
				current = current->next;
		}

		return count;
	}

	void Memory::setBounds(char *new_start, char *new_high) {
		start = new_start;
		high = new_high;
		end = new_start;
		realign((uintptr_t) start);
	}

	size_t Memory::getAllocated() const {
		return allocated;
	}

	size_t Memory::getUnallocated() const {
		return high - start - allocated;
	}
}

extern "C" void * malloc(size_t size) {
	printf("malloc(0x%lx)\n", size);
	if (global_memory == nullptr)
		return nullptr;
	return global_memory->allocate(size);
}

extern "C" void * calloc(size_t count, size_t size) {
	void *chunk = malloc(count * size);
	memset(chunk, 0, count * size);
	return chunk;
}

extern "C" void free(void *ptr) {
	if (global_memory)
		global_memory->free(ptr);
}

extern "C" int posix_memalign(void ** /* memptr */, size_t alignment, size_t /* size */) {
	// Return EINVAL if the alignment isn't zero or a power of two or is less than the size of a void pointer.
	if ((alignment & (alignment - 1)) != 0 || alignment < sizeof(void *))
		return 22; // EINVAL
	return 0;
}
