#pragma once

// Based on code from MINIX3.

extern uint8_t last_scancode;

namespace Thorn::PS2Keyboard {
	void onIRQ1();

	enum class InputPage: unsigned char {
		Invalid        = 0,
		GeneralDesktop = 0x01,
		Keyboard       = 0x07,
		LED            = 0x08,
		Buttons        = 0x09,
		Consumer       = 0x0c
	};

	enum class InputKey: short {
		Invalid = 0,
		KeyA = 0x0004,
		KeyB,
		KeyC,
		KeyD,
		KeyE,
		KeyF,
		KeyG,
		KeyH,
		KeyI,
		KeyJ,
		KeyK,
		KeyL,
		KeyM,
		KeyN,
		KeyO,
		KeyP,
		KeyQ,
		KeyR,
		KeyS,
		KeyT,
		KeyU,
		KeyV,
		KeyW,
		KeyX,
		KeyY,
		KeyZ,
		Key1,
		Key2,
		Key3,
		Key4,
		Key5,
		Key6,
		Key7,
		Key8,
		Key9,
		Key0,

		KeyEnter,
		KeyEscape,
		KeyBackspace,
		KeyTab,
		KeySpacebar,
		KeyDash,
		KeyEqual,
		KeyOpenBracket,
		KeyCloseBracket,
		KeyBackslash,

		KeyEurope1,
		KeySemicolon,
		KeyApostrophe,
		KeyGraveAccent,
		KeyComma,
		KeyPeriod,
		KeySlash,
		KeyCapsLock,

		KeyF1,
		KeyF2,
		KeyF3,
		KeyF4,
		KeyF5,
		KeyF6,
		KeyF7,
		KeyF8,
		KeyF9,
		KeyF10,
		KeyF11,
		KeyF12,

		KeyPrintScreen,
		KeyScrollLock,
		KeyPause,
		KeyInsert,
		KeyHome,
		KeyPageUp,
		KeyDelete,
		KeyEnd,
		KeyPageDown,
		KeyRightArrow,
		KeyLeftArrow,
		KeyDownArrow,
		KeyUpArrow,
		KeyNumLock,

		KeyKpSlash,
		KeyKpStar,
		KeyKpDash,
		KeyKpPlus,
		KeyKpEnter,
		KeyKp1,
		KeyKp2,
		KeyKp3,
		KeyKp4,
		KeyKp5,
		KeyKp6,
		KeyKp7,
		KeyKp8,
		KeyKp9,
		KeyKp0,
		KeyKpPeriod,

		KeyEurope2,
		KeyApplication,
		KeyPower,
		KeyKpEqual,

		KeyF13,
		KeyF14,
		KeyF15,
		KeyF16,
		KeyF17,
		KeyF18,
		KeyF19,
		KeyF20,
		KeyF21,
		KeyF22,
		KeyF23,
		KeyF24,

		KeyExecute,
		KeyHelp,
		KeyMenu,
		KeySelect,
		KeyStop,
		KeyAgain,
		KeyUndo,
		KeyCut,
		KeyCopy,
		KeyPaste,
		KeyFind,
		KeyMute,
		KeyVolumeUp,
		KeyVolumeDown,
		KeyLockingCapsLock,
		KeyLockingNumLock,
		KeyLockingScrollLock,
		KeyKpComma,
		KeyEqualSign,
		KeyI10L1,
		KeyI10L2,
		KeyI10L3,
		KeyI10L4,
		KeyI10L5,
		KeyI10L6,
		KeyI10L7,
		KeyI10L8,
		KeyI10L9,
		KeyLang1,
		KeyLang2,
		KeyLang3,
		KeyLang4,
		KeyLang5,
		KeyLang6,
		KeyLang7,
		KeyLang8,
		KeyLang9,
		KeyAltErase,
		KeySysReq,
		KeyCancel,
		KeyClear,
		KeyPrior,
		KeyReturn,
		KeySeparator,
		KeyOut,
		KeyOper,
		KeyClearAgain,
		KeyCrSel,
		KeyExSel,

		// 0x00A5 through 0x00AF are reserved

		KeyKp00 = 0x00B0,
		KeyKp000,
		KeyThousandsSep,
		KeyDecimalSep,
		KeyCurrencyUnit,
		KeyCurrencySubunit,
		KeyKpOpenParenthesis,
		KeyKpCloseParenthesis,
		KeyKpOpenBrace,
		KeyKpCloseBrace,
		KeyKpTab,
		KeyKpBackspace,
		KeyKpA,
		KeyKpB,
		KeyKpC,
		KeyKpD,
		KeyKpE,
		KeyKpF,
		KeyKpXor,
		KeyKpCaret,
		KeyKpPercent,
		KeyKpSmallerThen,
		KeyKpGreaterThen,
		KeyKpAmp,
		KeyKpDoubleAmp,
		KeyKpPipe,
		KeyKpDoublePipe,
		KeyKpColon,
		KeyKpNumber,
		KeyKpSpace,
		KeyKpAt,
		KeyKpExclamationMark,
		KeyKpMemStore,
		KeyKpMemRecall,
		KeyKpMemClear,
		KeyKpMemAdd,
		KeyKpMemSubtract,
		KeyKpMemMultiply,
		KeyKpMemDivide,
		KeyKpPlusMinus,
		KeyKpClear,
		KeyKpClearEntry,
		KeyKpBin,
		KeyKpOct,
		KeyKpDec,
		KeyKpHex,

		// 0x00DE and 0x00DF are reserved

		KeyLeftCtrl = 0x00E0,
		KeyLeftShift,
		KeyLeftAlt,
		KeyLeftGui,
		KeyRightCtrl,
		KeyRightShift,
		KeyRightAlt,
		KeyRightGui

		// 0x00E8 through 0xFFFF are reserved
	};

	struct Scanmap {
		InputPage page;
		InputKey key;
		Scanmap(): Scanmap(InputPage::Invalid, InputKey::Invalid) {}
		Scanmap(InputPage page_, InputKey key_): page(page_), key(key_) {}
	};
}
