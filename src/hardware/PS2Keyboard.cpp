#include <stdint.h>

#include "arch/x86_64/PIC.h"
#include "hardware/IDE.h"
#include "hardware/Ports.h"
#include "hardware/PS2Keyboard.h"
#include "lib/printf.h"
#include "memory/memset.h"

// #define PS2_KEYBOARD_DEBUG

uint8_t last_scancode = 0;

namespace Thorn::PS2Keyboard {
	const Scanmap scanmapNormal[0x80] = {
		/* 0x00 */ {},
		/* 0x01 */ {InputPage::Keyboard, InputKey::KeyEscape},
		/* 0x02 */ {InputPage::Keyboard, InputKey::Key1},
		/* 0x03 */ {InputPage::Keyboard, InputKey::Key2},
		/* 0x04 */ {InputPage::Keyboard, InputKey::Key3},
		/* 0x05 */ {InputPage::Keyboard, InputKey::Key4},
		/* 0x06 */ {InputPage::Keyboard, InputKey::Key5},
		/* 0x07 */ {InputPage::Keyboard, InputKey::Key6},
		/* 0x08 */ {InputPage::Keyboard, InputKey::Key7},
		/* 0x09 */ {InputPage::Keyboard, InputKey::Key8},
		/* 0x0a */ {InputPage::Keyboard, InputKey::Key9},
		/* 0x0b */ {InputPage::Keyboard, InputKey::Key0},
		/* 0x0c */ {InputPage::Keyboard, InputKey::KeyDash},
		/* 0x0d */ {InputPage::Keyboard, InputKey::KeyEqual},
		/* 0x0e */ {InputPage::Keyboard, InputKey::KeyBackspace},
		/* 0x0f */ {InputPage::Keyboard, InputKey::KeyTab},
		/* 0x10 */ {InputPage::Keyboard, InputKey::KeyQ},
		/* 0x11 */ {InputPage::Keyboard, InputKey::KeyW},
		/* 0x12 */ {InputPage::Keyboard, InputKey::KeyE},
		/* 0x13 */ {InputPage::Keyboard, InputKey::KeyR},
		/* 0x14 */ {InputPage::Keyboard, InputKey::KeyT},
		/* 0x15 */ {InputPage::Keyboard, InputKey::KeyY},
		/* 0x16 */ {InputPage::Keyboard, InputKey::KeyU},
		/* 0x17 */ {InputPage::Keyboard, InputKey::KeyI},
		/* 0x18 */ {InputPage::Keyboard, InputKey::KeyO},
		/* 0x19 */ {InputPage::Keyboard, InputKey::KeyP},
		/* 0x1a */ {InputPage::Keyboard, InputKey::KeyOpenBracket},
		/* 0x1b */ {InputPage::Keyboard, InputKey::KeyCloseBracket},
		/* 0x1c */ {InputPage::Keyboard, InputKey::KeyEnter},
		/* 0x1d */ {InputPage::Keyboard, InputKey::KeyLeftCtrl},
		/* 0x1e */ {InputPage::Keyboard, InputKey::KeyA},
		/* 0x1f */ {InputPage::Keyboard, InputKey::KeyS},
		/* 0x20 */ {InputPage::Keyboard, InputKey::KeyD},
		/* 0x21 */ {InputPage::Keyboard, InputKey::KeyF},
		/* 0x22 */ {InputPage::Keyboard, InputKey::KeyG},
		/* 0x23 */ {InputPage::Keyboard, InputKey::KeyH},
		/* 0x24 */ {InputPage::Keyboard, InputKey::KeyJ},
		/* 0x25 */ {InputPage::Keyboard, InputKey::KeyK},
		/* 0x26 */ {InputPage::Keyboard, InputKey::KeyL},
		/* 0x27 */ {InputPage::Keyboard, InputKey::KeySemicolon},
		/* 0x28 */ {InputPage::Keyboard, InputKey::KeyApostrophe},
		/* 0x29 */ {InputPage::Keyboard, InputKey::KeyGraveAccent},
		/* 0x2a */ {InputPage::Keyboard, InputKey::KeyLeftShift},
		/* 0x2b */ {InputPage::Keyboard, InputKey::KeyBackslash},
		/* 0x2c */ {InputPage::Keyboard, InputKey::KeyZ},
		/* 0x2d */ {InputPage::Keyboard, InputKey::KeyX},
		/* 0x2e */ {InputPage::Keyboard, InputKey::KeyC},
		/* 0x2f */ {InputPage::Keyboard, InputKey::KeyV},
		/* 0x30 */ {InputPage::Keyboard, InputKey::KeyB},
		/* 0x31 */ {InputPage::Keyboard, InputKey::KeyN},
		/* 0x32 */ {InputPage::Keyboard, InputKey::KeyM},
		/* 0x33 */ {InputPage::Keyboard, InputKey::KeyComma},
		/* 0x34 */ {InputPage::Keyboard, InputKey::KeyPeriod},
		/* 0x35 */ {InputPage::Keyboard, InputKey::KeySlash},
		/* 0x36 */ {InputPage::Keyboard, InputKey::KeyRightShift},
		/* 0x37 */ {InputPage::Keyboard, InputKey::KeyKpStar},
		/* 0x38 */ {InputPage::Keyboard, InputKey::KeyLeftAlt},
		/* 0x39 */ {InputPage::Keyboard, InputKey::KeySpacebar},
		/* 0x3a */ {InputPage::Keyboard, InputKey::KeyCapsLock},
		/* 0x3b */ {InputPage::Keyboard, InputKey::KeyF1},
		/* 0x3c */ {InputPage::Keyboard, InputKey::KeyF2},
		/* 0x3d */ {InputPage::Keyboard, InputKey::KeyF3},
		/* 0x3e */ {InputPage::Keyboard, InputKey::KeyF4},
		/* 0x3f */ {InputPage::Keyboard, InputKey::KeyF5},
		/* 0x40 */ {InputPage::Keyboard, InputKey::KeyF6},
		/* 0x41 */ {InputPage::Keyboard, InputKey::KeyF7},
		/* 0x42 */ {InputPage::Keyboard, InputKey::KeyF8},
		/* 0x43 */ {InputPage::Keyboard, InputKey::KeyF9},
		/* 0x44 */ {InputPage::Keyboard, InputKey::KeyF10},
		/* 0x45 */ {InputPage::Keyboard, InputKey::KeyNumLock},
		/* 0x46 */ {InputPage::Keyboard, InputKey::KeyScrollLock},
		/* 0x47 */ {InputPage::Keyboard, InputKey::KeyKp7},
		/* 0x48 */ {InputPage::Keyboard, InputKey::KeyKp8},
		/* 0x49 */ {InputPage::Keyboard, InputKey::KeyKp9},
		/* 0x4a */ {InputPage::Keyboard, InputKey::KeyKpDash},
		/* 0x4b */ {InputPage::Keyboard, InputKey::KeyKp4},
		/* 0x4c */ {InputPage::Keyboard, InputKey::KeyKp5},
		/* 0x4d */ {InputPage::Keyboard, InputKey::KeyKp6},
		/* 0x4e */ {InputPage::Keyboard, InputKey::KeyKpPlus},
		/* 0x4f */ {InputPage::Keyboard, InputKey::KeyKp1},
		/* 0x50 */ {InputPage::Keyboard, InputKey::KeyKp2},
		/* 0x51 */ {InputPage::Keyboard, InputKey::KeyKp3},
		/* 0x52 */ {InputPage::Keyboard, InputKey::KeyKp0},
		/* 0x53 */ {InputPage::Keyboard, InputKey::KeyKpPeriod},
		/* 0x54 */ {InputPage::Keyboard, InputKey::KeySysReq},
		/* 0x55 */ {},
		/* 0x56 */ {InputPage::Keyboard, InputKey::KeyEurope2},
		/* 0x57 */ {InputPage::Keyboard, InputKey::KeyF11},
		/* 0x58 */ {InputPage::Keyboard, InputKey::KeyF12},
		/* 0x59 */ {InputPage::Keyboard, InputKey::KeyKpEqual},
		/* 0x5a */ {},
		/* 0x5b */ {},
		/* 0x5c */ {InputPage::Keyboard, InputKey::KeyI10L2},
		/* 0x5d */ {},
		/* 0x5e */ {},
		/* 0x5f */ {},
		/* 0x60 */ {},
		/* 0x61 */ {},
		/* 0x62 */ {},
		/* 0x63 */ {},
		/* 0x64 */ {InputPage::Keyboard, InputKey::KeyF13},
		/* 0x65 */ {InputPage::Keyboard, InputKey::KeyF14},
		/* 0x66 */ {InputPage::Keyboard, InputKey::KeyF15},
		/* 0x67 */ {InputPage::Keyboard, InputKey::KeyF16},
		/* 0x68 */ {InputPage::Keyboard, InputKey::KeyF17},
		/* 0x69 */ {InputPage::Keyboard, InputKey::KeyF18},
		/* 0x6a */ {InputPage::Keyboard, InputKey::KeyF19},
		/* 0x6b */ {InputPage::Keyboard, InputKey::KeyF20},
		/* 0x6c */ {InputPage::Keyboard, InputKey::KeyF21},
		/* 0x6d */ {InputPage::Keyboard, InputKey::KeyF22},
		/* 0x6e */ {InputPage::Keyboard, InputKey::KeyF23},
		/* 0x6f */ {},
		/* 0x70 */ {InputPage::Keyboard, InputKey::KeyI10L2},
		/* 0x71 */ {InputPage::Keyboard, InputKey::KeyLang1}, // Release-only
		/* 0x72 */ {InputPage::Keyboard, InputKey::KeyLang1}, // Release-only
		/* 0x73 */ {InputPage::Keyboard, InputKey::KeyI10L1},
		/* 0x74 */ {},
		/* 0x75 */ {},
		/* 0x76 */ {InputPage::Keyboard, InputKey::KeyF24}, // Either F24 or LANG_5
		/* 0x77 */ {InputPage::Keyboard, InputKey::KeyLang4},
		/* 0x78 */ {InputPage::Keyboard, InputKey::KeyLang3},
		/* 0x79 */ {InputPage::Keyboard, InputKey::KeyI10L4},
		/* 0x7a */ {},
		/* 0x7b */ {InputPage::Keyboard, InputKey::KeyI10L5},
		/* 0x7c */ {},
		/* 0x7d */ {InputPage::Keyboard, InputKey::KeyI10L3},
		/* 0x7e */ {InputPage::Keyboard, InputKey::KeyEqualSign},
		/* 0x7f */ {}
	};

	const char *keyNames[0x80] = {
		/* 0x00 */ "Invalid",
		/* 0x01 */ "Escape",
		/* 0x02 */ "1",
		/* 0x03 */ "2",
		/* 0x04 */ "3",
		/* 0x05 */ "4",
		/* 0x06 */ "5",
		/* 0x07 */ "6",
		/* 0x08 */ "7",
		/* 0x09 */ "8",
		/* 0x0a */ "9",
		/* 0x0b */ "0",
		/* 0x0c */ "Dash",
		/* 0x0d */ "Equal",
		/* 0x0e */ "Backspace",
		/* 0x0f */ "Tab",
		/* 0x10 */ "Q",
		/* 0x11 */ "W",
		/* 0x12 */ "E",
		/* 0x13 */ "R",
		/* 0x14 */ "T",
		/* 0x15 */ "Y",
		/* 0x16 */ "U",
		/* 0x17 */ "I",
		/* 0x18 */ "O",
		/* 0x19 */ "P",
		/* 0x1a */ "OpenBracket",
		/* 0x1b */ "CloseBracket",
		/* 0x1c */ "Enter",
		/* 0x1d */ "LeftCtrl",
		/* 0x1e */ "A",
		/* 0x1f */ "S",
		/* 0x20 */ "D",
		/* 0x21 */ "F",
		/* 0x22 */ "G",
		/* 0x23 */ "H",
		/* 0x24 */ "J",
		/* 0x25 */ "K",
		/* 0x26 */ "L",
		/* 0x27 */ "Semicolon",
		/* 0x28 */ "Apostrophe",
		/* 0x29 */ "GraveAccent",
		/* 0x2a */ "LeftShift",
		/* 0x2b */ "Backslash",
		/* 0x2c */ "Z",
		/* 0x2d */ "X",
		/* 0x2e */ "C",
		/* 0x2f */ "V",
		/* 0x30 */ "B",
		/* 0x31 */ "N",
		/* 0x32 */ "M",
		/* 0x33 */ "Comma",
		/* 0x34 */ "Period",
		/* 0x35 */ "Slash",
		/* 0x36 */ "RightShift",
		/* 0x37 */ "KpStar",
		/* 0x38 */ "LeftAlt",
		/* 0x39 */ "Spacebar",
		/* 0x3a */ "CapsLock",
		/* 0x3b */ "F1",
		/* 0x3c */ "F2",
		/* 0x3d */ "F3",
		/* 0x3e */ "F4",
		/* 0x3f */ "F5",
		/* 0x40 */ "F6",
		/* 0x41 */ "F7",
		/* 0x42 */ "F8",
		/* 0x43 */ "F9",
		/* 0x44 */ "F10",
		/* 0x45 */ "NumLock",
		/* 0x46 */ "ScrollLock",
		/* 0x47 */ "Kp7",
		/* 0x48 */ "Kp8",
		/* 0x49 */ "Kp9",
		/* 0x4a */ "KpDash",
		/* 0x4b */ "Kp4",
		/* 0x4c */ "Kp5",
		/* 0x4d */ "Kp6",
		/* 0x4e */ "KpPlus",
		/* 0x4f */ "Kp1",
		/* 0x50 */ "Kp2",
		/* 0x51 */ "Kp3",
		/* 0x52 */ "Kp0",
		/* 0x53 */ "KpPeriod",
		/* 0x54 */ "SysReq",
		/* 0x55 */ "Invalid",
		/* 0x56 */ "Europe2",
		/* 0x57 */ "F11",
		/* 0x58 */ "F12",
		/* 0x59 */ "KpEqual",
		/* 0x5a */ "Invalid",
		/* 0x5b */ "Invalid",
		/* 0x5c */ "I10L2",
		/* 0x5d */ "Invalid",
		/* 0x5e */ "Invalid",
		/* 0x5f */ "Invalid",
		/* 0x60 */ "Invalid",
		/* 0x61 */ "Invalid",
		/* 0x62 */ "Invalid",
		/* 0x63 */ "Invalid",
		/* 0x64 */ "F13",
		/* 0x65 */ "F14",
		/* 0x66 */ "F15",
		/* 0x67 */ "F16",
		/* 0x68 */ "F17",
		/* 0x69 */ "F18",
		/* 0x6a */ "F19",
		/* 0x6b */ "F20",
		/* 0x6c */ "F21",
		/* 0x6d */ "F22",
		/* 0x6e */ "F23",
		/* 0x6f */ "Invalid",
		/* 0x70 */ "I10L2",
		/* 0x71 */ "Lang1",
		/* 0x72 */ "Lang1",
		/* 0x73 */ "I10L1",
		/* 0x74 */ "Invalid",
		/* 0x75 */ "Invalid",
		/* 0x76 */ "F24/Lang5",
		/* 0x77 */ "Lang4",
		/* 0x78 */ "Lang3",
		/* 0x79 */ "I10L4",
		/* 0x7a */ "Invalid",
		/* 0x7b */ "I10L5",
		/* 0x7c */ "Invalid",
		/* 0x7d */ "I10L3",
		/* 0x7e */ "EqualSign",
		/* 0x7f */ "Invalid"
	};

	void onIRQ1() {
		x86_64::PIC::sendEOI(1);
		uint8_t scancode = Thorn::Ports::inb(0x60);
		last_scancode = scancode;

#ifdef PS2_KEYBOARD_DEBUG
		if (scancode & 0x80)
			printf("%s (0x%x) up\n", keyNames[scancode & ~0x80], scancode & ~0x80);
		else
			printf("%s (0x%x) down\n", keyNames[scancode], scancode);
#endif
	}
}
