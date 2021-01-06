#include <libc.h>
#include <stdio.h>
#include "lib/printf.h"
#include "memory/Memory.h"

extern "C" void __stack_chk_fail() {
	printf("__stack_chk_fail()\n");
	for (;;);
}

extern "C" void * realloc(void *ptr, size_t size) {
	if (!global_memory) {
		printf("global_memory is missing");
		for (;;);
	}

	if (ptr == nullptr) {
		if (size == 0)
			return nullptr;
		return malloc(size);
	} else {
		if (size == 0) {
			free(ptr);
			return nullptr;
		}
	}

	void *new_area = malloc(size);
	if (!new_area) {
		printf("malloc(%lu) failed\n", size);
		for (;;);
	}

	auto *meta = global_memory->getBlock(ptr);
	if (meta == nullptr) {
		printf("getBlock returned null\n");
		for (;;);
	}

	if (meta->size < size)
		size = meta->size;
	
	char *char_area = (char *) new_area;
	char *old_area = (char *) ptr;

	for (size_t i = 0; i < size; ++i)
		char_area[i] = old_area[i];
	
	free(ptr);
	return new_area;
}

extern "C" int __sprintf_chk(char *str, int, size_t strlen, const char *format) {
	return snprintf(str, strlen, format);
}

extern "C" int fputs(const char *s, FILE *) {
	printf("%s", s);
	return 0;
}

extern "C" int fputc(int c, FILE *) {
	printf("%c", c);
	return c;
}
