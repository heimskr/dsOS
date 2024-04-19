#include <cstddef>

void exit() {
	asm("mov $1, %%rax; int $0x80" ::: "rax");
}

void print(const char *text, size_t length) {
	asm("mov $2, %%rax; mov %0, %%rdi; mov %1, %%rsi; int $0x80" :: "r"(text), "r"(length) : "rax", "rdi", "rsi");
}

int main() {
	print("Hello, World!\n", 14);
	exit();
}
