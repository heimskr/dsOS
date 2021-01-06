#ifndef MEMORY_MEMORY_H_
#define MEMORY_MEMORY_H_

#include "Defs.h"
#include "lib/printf.h"

#define MEMORY_ALIGN 32

void spin(size_t time = 3);

namespace DsOS {
	class Memory {
		public:
			struct BlockMeta {
				void *extra1;
				void *extra2;
				size_t size;
				BlockMeta *next;
				bool free;
				void *extra3;
				void *extra4;
			};

		private:
			size_t align;
			size_t allocated = 0;
			char *start, *high, *end;
			BlockMeta *base = nullptr;

			uintptr_t realign(uintptr_t);
			BlockMeta * findFreeBlock(BlockMeta * &last, size_t);
			BlockMeta * requestSpace(BlockMeta *last, size_t);
			void split(BlockMeta &, size_t);
			BlockMeta * getBlock(void *);
			int merge();

		public:
			Memory(const Memory &) = delete;
			Memory(Memory &&) = delete;

			Memory(char *start_, char *high_);
			Memory();

			Memory & operator=(const Memory &) = delete;
			Memory & operator=(Memory &&) = delete;

			void * allocate(size_t size, size_t alignment = 0);
			void free(void *);
			void setBounds(char *new_start, char *new_high);
			size_t getAllocated() const;
			size_t getUnallocated() const;
	};
}

extern "C" {
	void * malloc(size_t);
	void * calloc(size_t, size_t);
	void free(void *);
	int posix_memalign(void **memptr, size_t alignment, size_t size);
}

extern DsOS::Memory *global_memory;

#define MEMORY_OPERATORS_SET
#ifndef __cpp_exceptions
inline void * operator new(size_t size)   throw() { return malloc(size); }
inline void * operator new[](size_t size) throw() { return malloc(size); }
inline void * operator new(size_t, void *ptr)   throw() { return ptr; }
inline void * operator new[](size_t, void *ptr) throw() { return ptr; }
inline void operator delete(void *ptr)   throw() { free(ptr); }
inline void operator delete[](void *ptr) throw() { free(ptr); }
inline void operator delete(void *, void *)   throw() {}
inline void operator delete[](void *, void *) throw() {}
inline void operator delete(void *, unsigned long)   throw() {}
inline void operator delete[](void *, unsigned long) throw() {}
#else
inline void * operator new(size_t size)   { return malloc(size); }
inline void * operator new[](size_t size) { return malloc(size); }
inline void * operator new(size_t, void *ptr)   { return ptr; }
inline void * operator new[](size_t, void *ptr) { return ptr; }
inline void operator delete(void *ptr)   noexcept { free(ptr); }
inline void operator delete[](void *ptr) noexcept { free(ptr); }
inline void operator delete(void *, void *)   noexcept {}
inline void operator delete[](void *, void *) noexcept {}
inline void operator delete(void *, unsigned long)   noexcept {}
inline void operator delete[](void *, unsigned long) noexcept {}
#endif
#endif
