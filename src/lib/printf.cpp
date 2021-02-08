#include "hardware/Serial.h"
#include "Terminal.h"
#include "lib/printf.h"
#include <cstring>
#include <cstdint>

enum class Status {Scan, Decide, D, U, S, X, B};

bool printf_putc = true;

// static void signed_to_dec(char *out, char *&optr, const size_t max, long long int n);
static void unsigned_to_dec(char *out, char *&optr, const size_t max, long long unsigned int n);
static void num_to_hex(char *out, char *&optr, const size_t max, long long unsigned int n);
static void num_to_bin(char *out, char *&optr, const size_t max, long long unsigned int n);
static bool mappend(char *out, char *&optr, const size_t max, const char ch);

template <typename T>
static void signed_to_dec(char *out, char *&optr, const size_t max, T n) {
	if (out != nullptr && max <= static_cast<size_t>(optr - out))
		return;

	if (n == 0) {
		mappend(out, optr, max, '0');
		return;
	}

	char buffer[21] = {0};
	int i = 0;
	bool was_negative = n < 0;

	if (was_negative)
		n = -n;

	while (0 < n) {
		buffer[i++] = '0' + (n % 10);
		n /= 10;
	}

	if (was_negative && !mappend(out, optr, max, '-'))
		return;
	
	for (int j = i - 1; 0 <= j; --j)
		if (!mappend(out, optr, max, buffer[j]))
			return;
}

static bool printf_warned = false;

static bool mappend(char *out, char *&optr, const size_t max, const char ch) {
	if (out == nullptr) {
		if (!Thorn::Serial::init()) {
			if (!printf_warned) {
				Thorn::Terminal::write("Serial failed to initialize.\n");
				printf_warned = true;
				// for (;;) asm("hlt");
			}
		} else if (ch == '\0')
			Thorn::Serial::write("\e[1m\\0\e[22m");
		else
			Thorn::Serial::write(ch);
		if (printf_putc)
			Thorn::Terminal::putChar(ch);
		return true;
	}

	if (static_cast<size_t>(optr - out) < max) {
		*optr++ = ch;
		*optr = '\0';
		return true;
	}

	return false;
}

extern "C" int printf(const char *format, ...) {
	va_list list;
	va_start(list, format);
	const int printed = vsnprintf(nullptr, SIZE_MAX, format, list);
	va_end(list);
	return printed;
}

extern "C" int serprintf(const char *format, ...) {
	const bool old_putc = printf_putc;
	printf_putc = false;
	va_list list;
	va_start(list, format);
	const int printed = vsnprintf(nullptr, SIZE_MAX, format, list);
	va_end(list);
	printf_putc = old_putc;
	return printed;
}

extern "C" int sprintf(char *out, const char *format, ...) {
	va_list list;
	va_start(list, format);
	out[0] = '\0';
	const int printed = vsnprintf(out, SIZE_MAX, format, list);
	va_end(list);
	return printed;
}

extern "C" int snprintf(char *out, size_t max, const char *format, ...) {
	va_list list;
	va_start(list, format);
	out[0] = '\0';
	const int printed = vsnprintf(out, max, format, list);
	va_end(list);
	return printed;
}

#define APPEND(ch) do { if (!mappend(out, optr, max, (ch))) return printed; ++printed; } while(0)

extern "C" int vsnprintf(char *out, size_t max, const char *format, va_list list) {
	if (max == 0)
		return 0;

	const size_t format_length = strlen(format);

	size_t printed = 0;
	Status status = Status::Scan;

	char *optr = out;
	size_t i = 0;
	bool is_long = false;
	while (i < format_length) {
		if (status == Status::Scan) {
			char ch = format[i++];
			if (ch != '%')
				APPEND(ch);
			else
				status = Status::Decide;
		}

		if (status == Status::Decide) {
			if (i == format_length)
				return printed;
			char next = format[i++];
			if (next == 'l') {
				is_long = true;
				continue;
			} else {
				if (next == 'd') {
					status = Status::D;
				} else if (next == 'u') {
					status = Status::U;
				} else if (next == 's') {
					status = Status::S;
				} else if (next == 'x') {
					status = Status::X;
				} else if (next == 'b') {
					status = Status::B;
				} else if (next == 'c') {
					APPEND(va_arg(list, int));
					status = Status::Scan;
				} else if (next == 'y') {
					if (va_arg(list, int)) {
						APPEND('y'); APPEND('e'); APPEND('s');
					} else {
						APPEND('n'); APPEND('o');
					}
					status = Status::Scan;
				} else if (next == 'Y') {
					if (va_arg(list, int)) {
						APPEND('Y'); APPEND('e'); APPEND('s');
					} else {
						APPEND('N'); APPEND('o');
					}
					status = Status::Scan;
				} else if (next == '%') {
					APPEND('%');
					status = Status::Scan;
				}
			}
		}

		if (status == Status::D) {
			// TODO: padding and such
			if (is_long)
				signed_to_dec(out, optr, max, va_arg(list, long long int));
			else
				signed_to_dec(out, optr, max, va_arg(list, int));
			status = Status::Scan;
			is_long = false;
		} else if (status == Status::U) {
			if (is_long)
				unsigned_to_dec(out, optr, max, va_arg(list, long long unsigned int));
			else
				unsigned_to_dec(out, optr, max, va_arg(list, unsigned int));
			status = Status::Scan;
			is_long = false;
		} else if (status == Status::X) {
			if (is_long)
				num_to_hex(out, optr, max, va_arg(list, long long unsigned int));
			else
				num_to_hex(out, optr, max, va_arg(list, unsigned int));
			status = Status::Scan;
			is_long = false;
		} else if (status == Status::B) {
			if (is_long)
				num_to_bin(out, optr, max, va_arg(list, long long unsigned int));
			else
				num_to_bin(out, optr, max, va_arg(list, unsigned int));
			status = Status::Scan;
			is_long = false;
		} else if (status == Status::S) {
			const char *str_arg = va_arg(list, const char *);
			for (size_t i = 0; str_arg[i]; ++i)
				APPEND(str_arg[i]);
			status = Status::Scan;
		}
	}

	return printed;
}

static void unsigned_to_dec(char *out, char *&optr, const size_t max, long long unsigned int n) {
	if (out != nullptr && max <= static_cast<size_t>(optr - out))
		return;

	if (n == 0) {
		mappend(out, optr, max, '0');
		return;
	}

	char buffer[20] = {0};
	int i = 0;

	while (0 < n) {
		buffer[i++] = '0' + (n % 10);
		n /= 10;
	}

	for (int j = i - 1; 0 <= j; --j) {
		if (!mappend(out, optr, max, buffer[j]))
			return;
	}
}

static void num_to_hex(char *out, char *&optr, const size_t max, long long unsigned int n) {
	if (out != nullptr && max <= static_cast<size_t>(optr - out))
		return;

	if (n == 0) {
		mappend(out, optr, max, '0');
		return;
	}

	char buffer[16] = {0};
	int i = 0;

	while (0 < n) {
		char bits = n & 0xf;
		buffer[i++] = bits < 10? '0' + bits : ('a' + bits - 0xa);
		n >>= 4;
	}

	for (int j = i - 1; 0 <= j; --j) {
		if (!mappend(out, optr, max, buffer[j]))
			return;
	}
}

static void num_to_bin(char *out, char *&optr, const size_t max, long long unsigned int n) {
	if (out != nullptr && max <= static_cast<size_t>(optr - out))
		return;

	if (n == 0) {
		mappend(out, optr, max, '0');
		return;
	}

	char buffer[64] = {0};
	int i = 0;

	while (0 < n) {
		buffer[i++] = (n & 1)? '1' : '0';
		n >>= 1;
	}

	for (int j = i - 1; 0 <= j; --j) {
		if (!mappend(out, optr, max, buffer[j]))
			return;
	}
}
