#pragma once

#include <cstdint>
#include <string>

namespace Thorn::Keyboard {
	enum class InputKey: short {
		Invalid = 0, KeyA = 0x0004, KeyB, KeyC, KeyD, KeyE, KeyF, KeyG, KeyH, KeyI, KeyJ, KeyK, KeyL, KeyM, KeyN, KeyO,

		KeyP, KeyQ, KeyR, KeyS, KeyT, KeyU, KeyV, KeyW, KeyX, KeyY, KeyZ, Key1, Key2, Key3, Key4, Key5, Key6, Key7,
		Key8, Key9, Key0,

		KeyEnter, KeyEscape, KeyBackspace, KeyTab, KeySpacebar, KeyDash, KeyEqual, KeyOpenBracket, KeyCloseBracket,
		KeyBackslash,

		KeyEurope1, KeySemicolon, KeyApostrophe, KeyGraveAccent, KeyComma, KeyPeriod, KeySlash, KeyCapsLock,

		KeyF1, KeyF2, KeyF3, KeyF4, KeyF5, KeyF6, KeyF7, KeyF8, KeyF9, KeyF10, KeyF11, KeyF12,

		KeyPrintScreen, KeyScrollLock, KeyPause, KeyInsert, KeyHome, KeyPageUp, KeyDelete, KeyEnd, KeyPageDown,
		KeyRightArrow, KeyLeftArrow, KeyDownArrow, KeyUpArrow, KeyNumLock,

		KeyKpSlash, KeyKpStar, KeyKpDash, KeyKpPlus, KeyKpEnter, KeyKp1, KeyKp2, KeyKp3, KeyKp4, KeyKp5, KeyKp6, KeyKp7,
		KeyKp8, KeyKp9, KeyKp0, KeyKpPeriod,

		KeyEurope2, KeyApplication, KeyPower, KeyKpEqual,

		KeyF13, KeyF14, KeyF15, KeyF16, KeyF17, KeyF18, KeyF19, KeyF20, KeyF21, KeyF22, KeyF23, KeyF24,

		KeyExecute, KeyHelp, KeyMenu, KeySelect, KeyStop, KeyAgain, KeyUndo, KeyCut, KeyCopy, KeyPaste, KeyFind,
		KeyMute, KeyVolumeUp, KeyVolumeDown, KeyLockingCapsLock, KeyLockingNumLock, KeyLockingScrollLock, KeyKpComma,
		KeyEqualSign, KeyI10L1, KeyI10L2, KeyI10L3, KeyI10L4, KeyI10L5, KeyI10L6, KeyI10L7, KeyI10L8, KeyI10L9,
		KeyLang1, KeyLang2, KeyLang3, KeyLang4, KeyLang5, KeyLang6, KeyLang7, KeyLang8, KeyLang9, KeyAltErase,
		KeySysReq, KeyCancel, KeyClear, KeyPrior, KeyReturn, KeySeparator, KeyOut, KeyOper, KeyClearAgain, KeyCrSel,
		KeyExSel, KeyTilde, KeyUnderscore, KeyDoubleQuote, KeyQuestionMark,

		// 0x00a7 through 0x00af are reserved

		KeyKp00 = 0x00b0, KeyKp000, KeyThousandsSep, KeyDecimalSep, KeyCurrencyUnit, KeyCurrencySubunit,
		KeyKpOpenParenthesis, KeyKpCloseParenthesis, KeyKpOpenBrace, KeyKpCloseBrace, KeyKpTab, KeyKpBackspace, KeyKpA,
		KeyKpB, KeyKpC, KeyKpD, KeyKpE, KeyKpF, KeyKpXor, KeyKpCaret, KeyKpPercent, KeyKpSmallerThan, KeyKpGreaterThan,
		KeyKpAmp, KeyKpDoubleAmp, KeyKpPipe, KeyKpDoublePipe, KeyKpColon, KeyKpNumber, KeyKpSpace, KeyKpAt,
		KeyKpExclamationMark, KeyKpMemStore, KeyKpMemRecall, KeyKpMemClear, KeyKpMemAdd, KeyKpMemSubtract,
		KeyKpMemMultiply, KeyKpMemDivide, KeyKpPlusMinus, KeyKpClear, KeyKpClearEntry, KeyKpBin, KeyKpOct, KeyKpDec,
		KeyKpHex,

		// 0x00de and 0x00df are reserved

		KeyLeftCtrl = 0x00e0, KeyLeftShift, KeyLeftAlt, KeyLeftMeta, KeyRightCtrl, KeyRightShift, KeyRightAlt,
		KeyRightMeta

		// 0x00e8 through 0xffff are reserved
	};

	enum class Modifier: uint8_t {
		Ctrl = 1, Shift = 2, Alt = 4, Meta = 8
	};

	std::string modifierString(uint8_t);
	std::string modifierString();

	uint8_t getModifier(InputKey);
	bool hasModifier(Modifier);

	void onKey(InputKey, bool down);

	extern uint8_t modifiers;
	extern bool capslock;
	extern bool numlock;

	/** Applies shift/capslock transforms to an InputKey. */
	InputKey transform(InputKey);
	std::string toString(InputKey);

	extern const char *keyNames[232];
}
