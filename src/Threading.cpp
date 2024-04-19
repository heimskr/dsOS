#include "Threading.h"

namespace Thorn {
	namespace {
		thread_local ThreadID threadID = 0;
	}

	ThreadID currentThreadID() {
		return threadID;
	}

	void currentThreadID(ThreadID new_id) {
		threadID = new_id;
	}
}
