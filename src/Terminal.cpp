#include "Terminal.h"
#include "Util.h"

namespace DsOS {
	size_t Terminal::row = 0;
	size_t Terminal::column = 0;
	uint8_t Terminal::color = vgaEntryColor(VGAColor::LightGray, VGAColor::Black);
	uint16_t * Terminal::buffer = reinterpret_cast<uint16_t *>(0xB8000);

	void Terminal::clear() {
		for (size_t y = 0; y < VGA_HEIGHT; ++y)
			for (size_t x = 0; x < VGA_WIDTH; ++x)
				buffer[y * VGA_WIDTH + x] = vgaEntry(' ', color);
	}

	void Terminal::setColor(uint8_t color) {
		color = color;
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
		for (unsigned char i = 0; i < lines; ++i) {
			for (size_t j = 0; j < (VGA_HEIGHT - 1) * VGA_WIDTH; ++j)
				buffer[j] = buffer[j + VGA_WIDTH];
			for (size_t j = (VGA_HEIGHT - 1) * VGA_WIDTH; j < VGA_HEIGHT * VGA_WIDTH; ++j)
				buffer[j] = vgaEntry(' ', static_cast<unsigned char>(VGAColor::LightGray));
		}
	}

	void Terminal::write(const char *data, size_t size) {
		for (size_t i = 0; i < size; i++)
			putChar(data[i]);
	}

	void Terminal::write(const char *data) {
		write(data, Util::strlen(data));
	}
}
