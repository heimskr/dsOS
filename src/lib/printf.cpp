#include "hardware/Serial.h"
#include "Terminal.h"
#include "lib/printf.h"
#include <cstring>
#include <cstdint>
#include <iomanip>
#include <string>
#include <sstream>

enum class Status {Scan, Decide, D, U, S, X, B};

// static void signed_to_dec(char *out, char *&optr, const size_t max, long long int n);
// static std::string unsigned_to_dec(char *out, char *&optr, const size_t max, long long unsigned int n);
// static std::string num_to_hex(char *out, char *&optr, const size_t max, long long unsigned int n);
// static std::string num_to_bin(char *out, char *&optr, const size_t max, long long unsigned int n);
// static bool mappend(char *out, char *&optr, const size_t max, const char ch);

// template <typename T>
// static std::string signed_to_dec(char *out, char *&optr, const size_t max, T n) {
// 	if (out != nullptr && max <= static_cast<size_t>(optr - out))
// 		return;

// 	if (n == 0) {
// 		mappend(out, optr, max, '0');
// 		return;
// 	}

// 	char buffer[21] = {0};
// 	int i = 0;
// 	bool was_negative = n < 0;

// 	if (was_negative)
// 		n = -n;

// 	while (0 < n) {
// 		buffer[i++] = '0' + (n % 10);
// 		n /= 10;
// 	}

// 	if (was_negative && !mappend(out, optr, max, '-'))
// 		return;

// 	for (int j = i - 1; 0 <= j; --j)
// 		if (!mappend(out, optr, max, buffer[j]))
// 			return;
// }

static bool printf_warned = false;

// static bool mappend(char *out, char *&optr, const size_t max, const char ch) {
// 	if (out == nullptr) {
// 		if (!Thorn::Serial::init()) {
// 			if (!printf_warned) {
// 				Thorn::Terminal::write("Serial failed to initialize.\n");
// 				printf_warned = true;
// 			}
// 		} else if (ch == '\0')
// 			Thorn::Serial::write("\e[1m\\0\e[22m");
// 		else
// 			Thorn::Serial::write(ch);
// 		if (printf_putc)
// 			Thorn::Terminal::putChar(ch);
// 		return true;
// 	}

// 	if (static_cast<size_t>(optr - out) < max) {
// 		*optr++ = ch;
// 		*optr = '\0';
// 		return true;
// 	}

// 	return false;
// }

struct Flags {
	bool isLong = false;
	bool zero = false;
	bool minus = false;
	bool plus = false;
	bool space = false;
	bool apostrophe = false;
	bool hash = false;
	bool gettingWidth = true;
	int width = 0;

	void reset() {
		isLong = false;
		zero = false;
		minus = false;
		plus = false;
		space = false;
		apostrophe = false;
		hash = false;
		gettingWidth = true;
		width = 0;
	}
};

template <typename T>
static void write_decimal(std::stringstream &ss, Flags &flags, Status &status, T n) {
	std::stringstream substream;

	if (flags.apostrophe)
		substream.imbue(std::locale(""));

	if (status == Status::X)
		substream << std::hex;

	substream << n;

	// std::string stringified = long_flag?
	// 	std::to_string(va_arg(list, long long int)) : std::to_string(va_arg(list, int));

	std::string stringified = substream.str();

	if (0 <= n) {
		if (flags.plus)
			stringified = "+" + stringified;
		else if (flags.space)
			stringified = " " + stringified;
	}

	if (flags.minus)
		ss << std::right;

	if (flags.zero)
		ss << std::setfill('0');
	else
		ss << std::setfill(' '); // Probably redundant.

	// if (apostrophe_flag)
		// ss.imbue(std::locale(""));

	ss << std::setw(flags.width) << stringified;

	status = Status::Scan;
	flags.reset();
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

// #define APPEND(ch) do { if (!mappend(out, optr, max, (ch))) return printed; ++printed; } while(0)

extern "C" int vsnprintf(char *out, size_t max, const char *format, va_list list) {
	if (max == 0)
		return 0;

	std::stringstream ss;

	const size_t format_length = strlen(format);

	size_t printed = 0;
	Status status = Status::Scan;

	char *optr = out;
	size_t i = 0;
	Flags flags;

	while (i < format_length) {
		if (status == Status::Scan) {
			char ch = format[i++];
			if (ch != '%')
				ss << ch;
			else
				status = Status::Decide;
		}

		if (status == Status::Decide) {
			if (i == format_length)
				return printed;
			char next = format[i++];
			if (next == 'l') {
				flags.isLong = true;
			} else if (next == '0') {
				if (flags.gettingWidth)
					flags.width *= 10;
				else {
					flags.zero = true;
					flags.gettingWidth = true;
				}
			} else if ('1' <= next && next <= '9') {
				flags.gettingWidth = true;
				flags.width *= 10;
				flags.width += next - '0';
			} else if (next == '-') {
				flags.minus = true;
			} else if (next == '+') {
				flags.plus = true;
			} else if (next == ' ') {
				flags.space = true;
			} else if (next == '\'') {
				flags.apostrophe = true;
			} else if (next == '#') {
				flags.hash = true;
			} else if (next == 'd') {
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
				// APPEND(va_arg(list, int));
				ss << static_cast<char>(va_arg(list, int));
				status = Status::Scan;
			} else if (next == 'y') {
				if (va_arg(list, int))
					ss << "yes";
				else
					ss << "no";
				status = Status::Scan;
			} else if (next == 'Y') {
				if (va_arg(list, int))
					ss << "Yes";
				else
					ss << "No";
				status = Status::Scan;
			} else if (next == '%') {
				ss << '%';
				status = Status::Scan;
			}
		}

		if (status == Status::D || status == Status::X) { // signed
			if (flags.isLong)
				write_decimal(ss, flags, status, va_arg(list, long long int));
			else
				write_decimal(ss, flags, status, va_arg(list, int));
		} else if (status == Status::U) { // unsigned
			if (flags.isLong)
				unsigned_to_dec(out, optr, max, va_arg(list, long long unsigned int));
			else
				unsigned_to_dec(out, optr, max, va_arg(list, unsigned int));
			status = Status::Scan;
			flags.reset();
		} else if (status == Status::B) { // boolean
			if (flags.isLong)
				num_to_bin(out, optr, max, va_arg(list, long long unsigned int));
			else
				num_to_bin(out, optr, max, va_arg(list, unsigned int));
			status = Status::Scan;
			flags.reset();
		} else if (status == Status::S) { // string
			const char *str_arg = va_arg(list, const char *);
			for (size_t i = 0; str_arg[i]; ++i)
				APPEND(str_arg[i]);
			status = Status::Scan;
			flags.reset();
		}
	}

	std::string str = ss.str();

	if (!Thorn::Serial::init()) {
		if (!printf_warned) {
			Thorn::Terminal::write("Serial failed to initialize.\n");
			printf_warned = true;
		}
	} else
		Thorn::Serial::write(str.c_str());

	if (printf_putc)
		Thorn::Terminal::write(str.c_str());

	if (out)
		strncpy(out, str.c_str(), max);

	return printed;
}

// static void unsigned_to_dec(char *out, char *&optr, const size_t max, long long unsigned int n) {
// 	if (out != nullptr && max <= static_cast<size_t>(optr - out))
// 		return;

// 	if (n == 0) {
// 		mappend(out, optr, max, '0');
// 		return;
// 	}

// 	char buffer[20] = {0};
// 	int i = 0;

// 	while (0 < n) {
// 		buffer[i++] = '0' + (n % 10);
// 		n /= 10;
// 	}

// 	for (int j = i - 1; 0 <= j; --j) {
// 		if (!mappend(out, optr, max, buffer[j]))
// 			return;
// 	}
// }

// static void num_to_hex(char *out, char *&optr, const size_t max, long long unsigned int n) {
// 	if (out != nullptr && max <= static_cast<size_t>(optr - out))
// 		return;

// 	if (n == 0) {
// 		mappend(out, optr, max, '0');
// 		return;
// 	}

// 	char buffer[16] = {0};
// 	int i = 0;

// 	while (0 < n) {
// 		char bits = n & 0xf;
// 		buffer[i++] = bits < 10? '0' + bits : ('a' + bits - 0xa);
// 		n >>= 4;
// 	}

// 	for (int j = i - 1; 0 <= j; --j) {
// 		if (!mappend(out, optr, max, buffer[j]))
// 			return;
// 	}
// }

// static void num_to_bin(char *out, char *&optr, const size_t max, long long unsigned int n) {
// 	if (out != nullptr && max <= static_cast<size_t>(optr - out))
// 		return;

// 	if (n == 0) {
// 		mappend(out, optr, max, '0');
// 		return;
// 	}

// 	char buffer[64] = {0};
// 	int i = 0;

// 	while (0 < n) {
// 		buffer[i++] = (n & 1)? '1' : '0';
// 		n >>= 1;
// 	}

// 	for (int j = i - 1; 0 <= j; --j) {
// 		if (!mappend(out, optr, max, buffer[j]))
// 			return;
// 	}
// }
