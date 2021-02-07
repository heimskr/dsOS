#include <stdint.h>

#include "arch/x86_64/PIC.h"
#include "hardware/IDE.h"
#include "hardware/Ports.h"
#include "hardware/PS2Keyboard.h"
#include "lib/printf.h"
#include "memory/memset.h"

// #define PS2_KEYBOARD_DEBUG

volatile uint8_t last_scancode = 0;

namespace Thorn::PS2Keyboard {
	Scanmap scanmapNormal[0x80];

	void onIRQ1() {
		uint8_t scancode = Thorn::Ports::inb(0x60);
		last_scancode = scancode;

#ifdef PS2_KEYBOARD_DEBUG
		if (scancode & 0x80)
			printf("%s (0x%x) up\n", keyNames[scancode & ~0x80], scancode & ~0x80);
		else
			printf("%s (0x%x) down\n", keyNames[scancode], scancode);
#endif
	}

	void init() {
		scanmapNormal[0x00] = Scanmap();
		scanmapNormal[0x01] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyEscape);
		scanmapNormal[0x02] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key1);
		scanmapNormal[0x03] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key2);
		scanmapNormal[0x04] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key3);
		scanmapNormal[0x05] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key4);
		scanmapNormal[0x06] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key5);
		scanmapNormal[0x07] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key6);
		scanmapNormal[0x08] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key7);
		scanmapNormal[0x09] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key8);
		scanmapNormal[0x0a] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key9);
		scanmapNormal[0x0b] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::Key0);
		scanmapNormal[0x0c] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyDash);
		scanmapNormal[0x0d] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyEqual);
		scanmapNormal[0x0e] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyBackspace);
		scanmapNormal[0x0f] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyTab);
		scanmapNormal[0x10] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyQ);
		scanmapNormal[0x11] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyW);
		scanmapNormal[0x12] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyE);
		scanmapNormal[0x13] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyR);
		scanmapNormal[0x14] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyT);
		scanmapNormal[0x15] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyY);
		scanmapNormal[0x16] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyU);
		scanmapNormal[0x17] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyI);
		scanmapNormal[0x18] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyO);
		scanmapNormal[0x19] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyP);
		scanmapNormal[0x1a] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyOpenBracket);
		scanmapNormal[0x1b] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyCloseBracket);
		scanmapNormal[0x1c] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyEnter);
		scanmapNormal[0x1d] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyLeftCtrl);
		scanmapNormal[0x1e] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyA);
		scanmapNormal[0x1f] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyS);
		scanmapNormal[0x20] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyD);
		scanmapNormal[0x21] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF);
		scanmapNormal[0x22] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyG);
		scanmapNormal[0x23] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyH);
		scanmapNormal[0x24] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyJ);
		scanmapNormal[0x25] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyK);
		scanmapNormal[0x26] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyL);
		scanmapNormal[0x27] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeySemicolon);
		scanmapNormal[0x28] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyApostrophe);
		scanmapNormal[0x29] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyGraveAccent);
		scanmapNormal[0x2a] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyLeftShift);
		scanmapNormal[0x2b] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyBackslash);
		scanmapNormal[0x2c] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyZ);
		scanmapNormal[0x2d] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyX);
		scanmapNormal[0x2e] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyC);
		scanmapNormal[0x2f] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyV);
		scanmapNormal[0x30] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyB);
		scanmapNormal[0x31] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyN);
		scanmapNormal[0x32] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyM);
		scanmapNormal[0x33] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyComma);
		scanmapNormal[0x34] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyPeriod);
		scanmapNormal[0x35] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeySlash);
		scanmapNormal[0x36] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyRightShift);
		scanmapNormal[0x37] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKpStar);
		scanmapNormal[0x38] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyLeftAlt);
		scanmapNormal[0x39] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeySpacebar);
		scanmapNormal[0x3a] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyCapsLock);
		scanmapNormal[0x3b] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF1);
		scanmapNormal[0x3c] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF2);
		scanmapNormal[0x3d] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF3);
		scanmapNormal[0x3e] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF4);
		scanmapNormal[0x3f] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF5);
		scanmapNormal[0x40] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF6);
		scanmapNormal[0x41] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF7);
		scanmapNormal[0x42] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF8);
		scanmapNormal[0x43] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF9);
		scanmapNormal[0x44] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF10);
		scanmapNormal[0x45] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyNumLock);
		scanmapNormal[0x46] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyScrollLock);
		scanmapNormal[0x47] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp7);
		scanmapNormal[0x48] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp8);
		scanmapNormal[0x49] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp9);
		scanmapNormal[0x4a] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKpDash);
		scanmapNormal[0x4b] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp4);
		scanmapNormal[0x4c] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp5);
		scanmapNormal[0x4d] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp6);
		scanmapNormal[0x4e] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKpPlus);
		scanmapNormal[0x4f] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp1);
		scanmapNormal[0x50] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp2);
		scanmapNormal[0x51] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp3);
		scanmapNormal[0x52] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKp0);
		scanmapNormal[0x53] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKpPeriod);
		scanmapNormal[0x54] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeySysReq);
		scanmapNormal[0x55] = Scanmap();
		scanmapNormal[0x56] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyEurope2);
		scanmapNormal[0x57] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF11);
		scanmapNormal[0x58] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF12);
		scanmapNormal[0x59] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyKpEqual);
		scanmapNormal[0x5a] = Scanmap();
		scanmapNormal[0x5b] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyLeftMeta);
		scanmapNormal[0x5c] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyI10L2);
		scanmapNormal[0x5d] = Scanmap();
		scanmapNormal[0x5e] = Scanmap();
		scanmapNormal[0x5f] = Scanmap();
		scanmapNormal[0x60] = Scanmap();
		scanmapNormal[0x61] = Scanmap();
		scanmapNormal[0x62] = Scanmap();
		scanmapNormal[0x63] = Scanmap();
		scanmapNormal[0x64] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF13);
		scanmapNormal[0x65] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF14);
		scanmapNormal[0x66] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF15);
		scanmapNormal[0x67] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF16);
		scanmapNormal[0x68] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF17);
		scanmapNormal[0x69] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF18);
		scanmapNormal[0x6a] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF19);
		scanmapNormal[0x6b] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF20);
		scanmapNormal[0x6c] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF21);
		scanmapNormal[0x6d] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF22);
		scanmapNormal[0x6e] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF23);
		scanmapNormal[0x6f] = Scanmap();
		scanmapNormal[0x70] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyI10L2);
		scanmapNormal[0x71] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyLang1); // Release-only
		scanmapNormal[0x72] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyLang1); // Release-only
		scanmapNormal[0x73] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyI10L1);
		scanmapNormal[0x74] = Scanmap();
		scanmapNormal[0x75] = Scanmap();
		scanmapNormal[0x76] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyF24); // Either F24 or LANG_5
		scanmapNormal[0x77] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyLang4);
		scanmapNormal[0x78] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyLang3);
		scanmapNormal[0x79] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyI10L4);
		scanmapNormal[0x7a] = Scanmap();
		scanmapNormal[0x7b] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyI10L5);
		scanmapNormal[0x7c] = Scanmap();
		scanmapNormal[0x7d] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyI10L3);
		scanmapNormal[0x7e] = Scanmap(InputPage::Keyboard, Keyboard::InputKey::KeyEqualSign);
		scanmapNormal[0x7f] = Scanmap();
	};
}
