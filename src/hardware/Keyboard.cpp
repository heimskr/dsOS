#include "hardware/Keyboard.h"
#include "Kernel.h"

namespace Thorn::Keyboard {
	uint8_t modifiers = 0;
	bool capslock = false;
	bool numlock = true;

	void onKey(InputKey key, bool down) {
		if (key == InputKey::Invalid)
			return;

		if (key == InputKey::KeyCapsLock && down)
			capslock = !capslock;
		if (key == InputKey::KeyNumLock && down)
			numlock = !numlock;

		if (down) {
			switch (key) {
				case InputKey::KeyLeftShift:
				case InputKey::KeyRightShift:
					modifiers |= static_cast<uint8_t>(Modifier::Shift);
					break;
				case InputKey::KeyLeftAlt:
				case InputKey::KeyRightAlt:
					modifiers |= static_cast<uint8_t>(Modifier::Alt);
					break;
				case InputKey::KeyLeftCtrl:
				case InputKey::KeyRightCtrl:
					modifiers |= static_cast<uint8_t>(Modifier::Ctrl);
					break;
				case InputKey::KeyLeftMeta:
				case InputKey::KeyRightMeta:
					modifiers |= static_cast<uint8_t>(Modifier::Meta);
					break;
				default:
					break;
			}
		} else {
			switch (key) {
				case InputKey::KeyLeftShift:
				case InputKey::KeyRightShift:
					modifiers &= ~static_cast<uint8_t>(Modifier::Shift);
					break;
				case InputKey::KeyLeftAlt:
				case InputKey::KeyRightAlt:
					modifiers &= ~static_cast<uint8_t>(Modifier::Alt);
					break;
				case InputKey::KeyLeftCtrl:
				case InputKey::KeyRightCtrl:
					modifiers &= ~static_cast<uint8_t>(Modifier::Ctrl);
					break;
				case InputKey::KeyLeftMeta:
				case InputKey::KeyRightMeta:
					modifiers &= ~static_cast<uint8_t>(Modifier::Meta);
					break;
				default:
					break;
			}
		}

		Kernel::getInstance().onKey(transform(key), down);
	}

	std::string modifierString(uint8_t mods) {
		std::string out;
		if (mods & static_cast<uint8_t>(Modifier::Ctrl)) {
			if (!out.empty()) out.push_back(' ');
			out += "ctrl";
		}
		if (mods & static_cast<uint8_t>(Modifier::Shift)) {
			if (!out.empty()) out.push_back(' ');
			out += "shift";
		}
		if (mods & static_cast<uint8_t>(Modifier::Alt)) {
			if (!out.empty()) out.push_back(' ');
			out += "alt";
		}
		if (mods & static_cast<uint8_t>(Modifier::Meta)) {
			if (!out.empty()) out.push_back(' ');
			out += "meta";
		}
		return out;
	}

	std::string modifierString() {
		return modifierString(modifiers);
	}
	
	uint8_t getModifier(InputKey key) {
		switch (key) {
			case InputKey::KeyLeftShift:
			case InputKey::KeyRightShift:
				return static_cast<uint8_t>(Modifier::Shift);
			case InputKey::KeyLeftAlt:
			case InputKey::KeyRightAlt:
				return static_cast<uint8_t>(Modifier::Alt);
			case InputKey::KeyLeftCtrl:
			case InputKey::KeyRightCtrl:
				return static_cast<uint8_t>(Modifier::Ctrl);
			case InputKey::KeyLeftMeta:
			case InputKey::KeyRightMeta:
				return static_cast<uint8_t>(Modifier::Meta);
			default:
				return 0;
		}
	}

	bool hasModifier(Modifier modifier) {
		return (modifiers & static_cast<uint8_t>(modifier)) != 0;
	}

	bool isModifier(Modifier modifier) {
		return modifiers == static_cast<uint8_t>(modifier);
	}

	InputKey transform(InputKey key) {
		if (numlock) {
			switch (key) {
				case InputKey::KeyKp0: key = InputKey::KeyInsert; break;
				case InputKey::KeyKp1: key = InputKey::KeyEnd; break;
				case InputKey::KeyKp2: key = InputKey::KeyDownArrow; break;
				case InputKey::KeyKp3: key = InputKey::KeyPageDown; break;
				case InputKey::KeyKp4: key = InputKey::KeyLeftArrow; break;
				case InputKey::KeyKp6: key = InputKey::KeyRightArrow; break;
				case InputKey::KeyKp7: key = InputKey::KeyHome; break;
				case InputKey::KeyKp8: key = InputKey::KeyUpArrow; break;
				case InputKey::KeyKp9: key = InputKey::KeyPageUp; break;
				default: break;
			}
		}

		if (hasModifier(Modifier::Shift)) {
			switch (key) {
				case InputKey::KeyGraveAccent: return InputKey::KeyTilde;
				case InputKey::Key1: return InputKey::KeyKpExclamationMark;
				case InputKey::Key2: return InputKey::KeyKpAt;
				case InputKey::Key3: return InputKey::KeyKpNumber;
				case InputKey::Key4: return InputKey::KeyCurrencyUnit;
				case InputKey::Key5: return InputKey::KeyKpPercent;
				case InputKey::Key6: return InputKey::KeyKpCaret;
				case InputKey::Key7: return InputKey::KeyKpAmp;
				case InputKey::Key8: return InputKey::KeyKpStar;
				case InputKey::Key9: return InputKey::KeyKpOpenParenthesis;
				case InputKey::Key0: return InputKey::KeyKpCloseParenthesis;
				case InputKey::KeyDash:
				case InputKey::KeyKpDash: return InputKey::KeyUnderscore;
				case InputKey::KeyEqual:
				case InputKey::KeyKpEqual:
				case InputKey::KeyEqualSign: return InputKey::KeyKpPlus;
				case InputKey::KeyOpenBracket: return InputKey::KeyKpOpenBrace;
				case InputKey::KeyCloseBracket: return InputKey::KeyKpCloseBrace;
				case InputKey::KeySemicolon: return InputKey::KeyKpColon;
				case InputKey::KeyApostrophe: return InputKey::KeyDoubleQuote;
				case InputKey::KeyComma: return InputKey::KeyKpSmallerThan;
				case InputKey::KeyPeriod: return InputKey::KeyKpGreaterThan;
				case InputKey::KeySlash: return InputKey::KeyQuestionMark;
				case InputKey::KeyBackslash: return InputKey::KeyKpPipe;
				default: break;
			}
		}
		
		return key;
	}
		

	std::string toString(InputKey key) {
		short casted = static_cast<short>(key);
		if (casted < static_cast<short>(InputKey::KeyA) || static_cast<short>(InputKey::KeyZ) < casted)
			return keyNames[casted];
		
		char ch = keyNames[casted][0];
		if (!!(modifiers & static_cast<uint8_t>(Modifier::Shift)) ^ capslock) {
			return std::string(1, ch + 'A' - 'a');
		}

		return std::string(1, ch);
	}

	const char *keyNames[232] = {
		"Invalid",
		"Invalid",
		"Invalid",
		"Invalid",
		"a",
		"b",
		"c",
		"d",
		"e",
		"f",
		"g",
		"h",
		"i",
		"j",
		"k",
		"l",
		"m",
		"n",
		"o",
		"p",
		"q",
		"r",
		"s",
		"t",
		"u",
		"v",
		"w",
		"x",
		"y",
		"z",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"0",
		"Enter",
		"Escape",
		"Backspace",
		"\t", // KeyTab
		" ", // KeySpacebar
		"-", // KeyDash
		"=", // KeyEqual
		"[", // KeyOpenBracket
		"]", // KeyCloseBracket
		"\\", // KeyBackslash
		"Europe1",
		";", // KeySemicolon
		"'", // KeyApostrophe
		"`", // KeyGrave
		",", // KeyComma
		".", // KeyPeriod
		"/", // KeySlash
		"CapsLock",
		"F1",
		"F2",
		"F3",
		"F4",
		"F5",
		"F6",
		"F7",
		"F8",
		"F9",
		"F10",
		"F11",
		"F12",
		"PrintScreen",
		"ScrollLock",
		"Pause",
		"Insert",
		"Home",
		"PageUp",
		"Delete",
		"End",
		"PageDown",
		"RightArrow",
		"LeftArrow",
		"DownArrow",
		"UpArrow",
		"NumLock",
		"/", // KeyKpSlash
		"*", // KeyKpStar
		"-", // KeyKpDash
		"+", // KeyPlus
		"KpEnter",
		"1", // KeyKp1
		"2", // KeyKp2
		"3", // KeyKp3
		"4", // KeyKp4
		"5", // KeyKp5
		"6", // KeyKp6
		"7", // KeyKp7
		"8", // KeyKp8
		"9", // KeyKp9
		"0", // KeyKp0
		".", // Keyperiod
		"Europe2",
		"Application",
		"Power",
		"=", // KeyKpEqual
		"F13",
		"F14",
		"F15",
		"F16",
		"F17",
		"F18",
		"F19",
		"F20",
		"F21",
		"F22",
		"F23",
		"F24",
		"Execute",
		"Help",
		"Menu",
		"Select",
		"Stop",
		"Again",
		"Undo",
		"Cut",
		"Copy",
		"Paste",
		"Find",
		"Mute",
		"VolumeUp",
		"VolumeDown",
		"LockingCapsLock",
		"LockingNumLock",
		"LockingScrollLock",
		",", // KeyKpComma
		"=", // KeyEqualSign
		"I10L1",
		"I10L2",
		"I10L3",
		"I10L4",
		"I10L5",
		"I10L6",
		"I10L7",
		"I10L8",
		"I10L9",
		"Lang1",
		"Lang2",
		"Lang3",
		"Lang4",
		"Lang5",
		"Lang6",
		"Lang7",
		"Lang8",
		"Lang9",
		"AltErase",
		"SysReq",
		"Cancel",
		"Clear",
		"Prior",
		"Return",
		"Separator",
		"Out",
		"Oper",
		"ClearAgain",
		"CrSel",
		"ExSel",
		"~",
		"_",
		"\"",
		"?",
		"Invalid",
		"Invalid",
		"Invalid",
		"Invalid",
		"Invalid",
		"Invalid",
		"Invalid",
		"Kp00",
		"Kp000",
		"ThousandsSep",
		"DecimalSep",
		"$", // KeyCurrencyUnit
		"CurrencySubunit",
		"(", // KeyKpOpenParenthesis
		")", // KeyKpCloseParenthesis
		"{", // KeyKpOpenBrace
		"}", // KeyKpCloseBrace
		"\t", // KeyKpTab
		"KpBackspace",
		"KpA",
		"KpB",
		"KpC",
		"KpD",
		"KpE",
		"KpF",
		"^", // KeyKpXor
		"^", // KeyKpCaret
		"%", // KeyKpPercent
		"<", // KeyKpSmallerThan
		">", // KeyKpGreaterThan
		"&", // KeyKpAmp
		"&&", // KeyKpDoubleAmp
		"|", // KeyKpPipe
		"||", // KeyKpDoublePipe
		":", // KeyKpColon
		"#", // KeyKpNumber
		" ", // KeyKpSpace
		"@", // KeyKpAt
		"!", // KeyKpExclamationMark
		"KpMemStore",
		"KpMemRecall",
		"KpMemClear",
		"KpMemAdd",
		"KpMemSubtract",
		"KpMemMultiply",
		"KpMemDivide",
		"KpPlusMinus",
		"KpClear",
		"KpClearEntry",
		"KpBin",
		"KpOct",
		"KpDec",
		"KpHex",
		"Invalid",
		"Invalid",
		"LeftCtrl",
		"LeftShift",
		"LeftAlt",
		"LeftMeta",
		"RightCtrl",
		"RightShift",
		"RightAlt",
		"RightMeta"
	};
}