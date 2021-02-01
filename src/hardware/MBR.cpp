#include "hardware/MBR.h"

namespace Thorn {
	bool MBR::indicatesGPT() const {
		return firstEntry.type == 0xee && secondEntry.type == 0 && thirdEntry.type == 0 && fourthEntry.type == 0;
	}
}
