#pragma once

#include "Mutex.h"

namespace Thorn {
	template <typename T, typename M = Mutex>
	class Lockable {
		public:
			template <typename... Args>
			Lockable(Args &&...args):
				object(std::forward<Args>(args)...) {}

			T & get(Lock<M> &lock) {
				lock = Lock<M>(mutex);
				return object;
			}

			const T & get(Lock<M> &lock) const {
				lock = Lock<M>(mutex);
				return object;
			}

		private:
			T object;
			mutable M mutex;
	};
}
