#pragma once

#include <queue>

#include "hardware/Keyboard.h"

// Based on code from MINIX3.

extern std::queue<uint8_t> scancodes_fifo;

namespace Thorn::PS2Keyboard {
	void onIRQ1();
	void init();

	enum class InputPage: unsigned char {
		Invalid        = 0x00,
		GeneralDesktop = 0x01,
		Keyboard       = 0x07,
		LED            = 0x08,
		Buttons        = 0x09,
		Consumer       = 0x0c,
	};

	struct Scanmap {
		InputPage page;
		Keyboard::InputKey key;
		Scanmap(): Scanmap(InputPage::Invalid, Keyboard::InputKey::Invalid) {}
		Scanmap(InputPage page_, Keyboard::InputKey key_): page(page_), key(key_) {}
	};

	extern Scanmap scanmapNormal[0x80];
}
