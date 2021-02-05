#include <string.h>

#include "Terminal.h"
#include "ThornUtil.h"
#include "hardware/Serial.h"

namespace Thorn {
	size_t Terminal::row = 0;
	size_t Terminal::column = 0;
	uint8_t Terminal::color = vgaEntryColor(VGAColor::LightGray, VGAColor::Black);
	uint8_t Terminal::tabSize = 4;
	uint16_t * Terminal::buffer = reinterpret_cast<uint16_t *>(0xB8000);

	void Terminal::clear() {
		Serial::write("\e[2m[clear]\e[22m");
		memset(buffer, 0, VGA_HEIGHT * VGA_WIDTH);
		row = column = 0;
	}

	void Terminal::setColor(uint8_t color_) {
		color = color_;
	}

	void Terminal::putEntryAt(char c, uint8_t color, size_t x, size_t y) {
		const size_t index = y * VGA_WIDTH + x;
		buffer[index] = vgaEntry(c, color);
	}

	void Terminal::putChar(char c) {
		switch (c) {
			case '\n':
				nextLine();
				break;
			case '\r':
				column = 0;
				break;
			case '\t':
				column = (column + tabSize) & ~(tabSize - 1);
				break;
			default:
				putEntryAt(c, color, column, row);
				if (++column == VGA_WIDTH)
					nextLine();
		}
	}

	void Terminal::nextLine() {
		column = 0;
		if (VGA_HEIGHT <= ++row) {
			scrollUp(1);
			--row;
		}
	}

	void Terminal::scrollUp(unsigned char lines) {
		for (uint8_t i = 0; i < lines; ++i) {
			for (uint8_t j = 0; j < VGA_HEIGHT - 1; ++j)
				memcpy(&buffer[j * VGA_WIDTH], &buffer[(j + 1) * VGA_WIDTH], VGA_WIDTH * sizeof(buffer[0]));
			for (size_t j = (VGA_HEIGHT - 1) * VGA_WIDTH; j < VGA_HEIGHT * VGA_WIDTH; ++j)
				buffer[j] = vgaEntry(' ', vgaEntryColor(VGAColor::LightGray, VGAColor::Black));
		}
	}

	void Terminal::write(const char *data, size_t size) {
		for (size_t i = 0; i < size; i++)
			putChar(data[i]);
	}

	void Terminal::write(const char *data) {
		for (size_t i = 0; data[i]; ++i)
			putChar(data[i]);
	}
}
