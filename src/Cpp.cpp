#include "Terminal.h"

extern "C" void __cxa_pure_virtual() {
	using namespace DsOS;
	Terminal::setColor(Terminal::vgaEntryColor(Terminal::VGAColor::White, Terminal::VGAColor::Red));
	Terminal::row = Terminal::column = 0;
	Terminal::write("Pure virtual function call!");
	while (1);
}
