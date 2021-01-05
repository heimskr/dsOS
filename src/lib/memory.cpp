#include "lib/atomic"
#include "lib/memory"

// Based on code from the LLVM project (https://llvm.org).

namespace std {
	shared_count::~shared_count() {}
	shared_weak_count::~shared_weak_count() {}	

	void shared_weak_count::release_weak() noexcept {
		if (weak_owners == 0) // TODO: atomic
			on_zero_shared_weak();
		else if (--weak_owners == -1) // TODO: atomic
			on_zero_shared_weak();
	}

	shared_weak_count * shared_weak_count::lock() noexcept {
		long object_owners = shared_owners; // TODO: atomic
		while (object_owners != -1) {
			if (__atomic_compare_exchange_n(&shared_owners, &object_owners, object_owners + 1, true, atomic_order::seq, atomic_order::seq))
				return this;
		}
		return nullptr;
	}
}
