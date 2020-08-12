#include "Terminal.h"
#include "Util.h"

namespace DsOS {
	void Terminal::init() {
		Terminal::row = 0;
		Terminal::column = 0;
		Terminal::color = vgaEntryColor(VGAColor::LightGray, VGAColor::Black);
		Terminal::buffer = reinterpret_cast<uint16_t *>(0xB8000);

		for (size_t y = 0; y < VGA_HEIGHT; y++)
			for (size_t x = 0; x < VGA_WIDTH; x++) {
				const size_t index = y * VGA_WIDTH + x;
				Terminal::buffer[index] = vgaEntry(' ', Terminal::color);
			}
	}

	void Terminal::setColor(uint8_t color) {
		Terminal::color = color;
	}

	void Terminal::putEntryAt(char c, uint8_t color, size_t x, size_t y) {
		const size_t index = y * VGA_WIDTH + x;
		Terminal::buffer[index] = vgaEntry(c, color);
	}

	void Terminal::putChar(char c) {
		switch (c) {
			case '\n':
				if (++Terminal::row == VGA_HEIGHT)
					Terminal::row = 0;
				Terminal::column = 0;
				break;
			case '\r':
				Terminal::column = 0;
				break;
			default:
				Terminal::putEntryAt(c, Terminal::color, Terminal::column, Terminal::row);
				if (++Terminal::column == VGA_WIDTH) {
					Terminal::column = 0;
					if (++Terminal::row == VGA_HEIGHT)
						Terminal::row = 0;
				}
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
