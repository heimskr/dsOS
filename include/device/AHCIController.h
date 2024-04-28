#pragma once

#include "device/Storage.h"

namespace Thorn {
	namespace AHCI { class Controller; }

	class AHCIController {
		private:
			AHCI::Controller *controller = nullptr;

		public:
			AHCIController(AHCI::Controller *controller_):
				controller(controller_) {}
	};
}
