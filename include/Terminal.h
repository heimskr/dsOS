#ifndef TERMINAL_H_
#define TERMINAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

namespace DsOS {
	struct Terminal {
		static constexpr size_t VGA_WIDTH = 80;
		static constexpr size_t VGA_HEIGHT = 25;
		static size_t row;
		static size_t column;
		static uint8_t color;
		static uint16_t *buffer;

		enum class VGAColor: int {
			Black = 0,
			Blue = 1,
			Green = 2,
			Cyan = 3,
			Red = 4,
			Magenta = 5,
			Brown = 6,
			LightGray = 7,
			DarkGray = 8,
			LightBlue = 9,
			LightGreen = 10,
			LightCyan = 11,
			LightRed = 12,
			LightMagenta = 13,
			LightBrown = 14,
			White = 15,
		};

		static constexpr uint8_t vgaEntryColor(VGAColor fg, VGAColor bg) {
			return static_cast<int>(fg) | static_cast<int>(bg) << 4;
		}

		static constexpr uint16_t vgaEntry(unsigned char uc, uint8_t color) {
			return static_cast<uint16_t>(uc) | static_cast<uint16_t>(color) << 8;
		}

		static void clear();
		static void setColor(uint8_t);
		static void putEntryAt(char c, uint8_t color, size_t x, size_t y);
		static void putChar(char);
		static void nextLine();
		static void scrollUp(unsigned char lines);
		static void write(const char *, size_t);
		static void write(const char *);
	};
}

#endif
