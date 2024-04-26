#pragma once

#include "arch/x86_64/APIC.h"

#include "Assert.h"
#include "Atomic.h"
#include "Threading.h"

namespace Thorn {
	class Mutex {
		public:
			constexpr static uint32_t Frequency = 1'000'000;

			void lock() {
				while (locked.exchange(true))
					x86_64::APIC::wait(1, Frequency);
			}

			bool try_lock() {
				return !locked.exchange(true);
			}

			void unlock() {
				if (!locked.exchange(false)) {
					assert(!"Mutex is locked");
				}
			}

			bool try_unlock() {
				return locked.exchange(false);
			}

		private:
			Atomic<bool> locked;
	};

	class RecursiveMutex {
		public:
			void lock() {
				submutex.lock();

				if (!locked && owner == currentThreadID()) {
					++depth;
					submutex.unlock();
					return;
				}

				submutex.unlock();

				while (locked.exchange(true))
					x86_64::APIC::wait(1, Mutex::Frequency);

				submutex.lock();
				owner = currentThreadID();
				depth = 1;
				submutex.unlock();
			}

			bool try_lock() {
				submutex.lock();

				if (locked && owner == currentThreadID()) {
					++depth;
					submutex.unlock();
					return true;
				}

				bool out = !locked.exchange(true);

				if (out) {
					owner = currentThreadID();
					depth = 1;
				}

				submutex.unlock();
				return out;
			}

			void unlock() {
				submutex.lock();

				if (locked) {
					if (owner == currentThreadID()) {
						assert(depth > 0);
						if (--depth == 0) {
							owner = 0;
							locked = false;
						}
					} else {
						assert(!"RecursiveMutex is locked by current thread");
					}
				} else {
					assert(!"RecursiveMutex is locked");
				}

				submutex.unlock();
			}

			bool try_unlock() {
				submutex.lock();

				if (locked && owner == currentThreadID()) {
					assert(depth > 0);
					if (--depth == 0) {
						owner = 0;
						locked = false;
						submutex.unlock();
						return true;
					}
				}

				submutex.unlock();
				return false;
			}

		private:
			Mutex submutex;
			Atomic<bool> locked;
			ThreadID owner = 0;
			int depth = 0;
	};

	template <typename M>
	class Lock {
		public:
			Lock() = default;

			Lock(M &mutex_): mutex(&mutex_) {
				lock();
			}

			Lock(const Lock<M> &) = delete;

			Lock(Lock<M> &&other) {
				mutex = other.mutex;
				other.mutex = nullptr;
			}

			Lock<M> & operator=(const Lock<M> &) = delete;

			Lock<M> & operator=(Lock<M> &&other) {
				mutex = other.mutex;
				other.mutex = nullptr;
				return *this;
			}

			~Lock() {
				if (mutex)
					unlock();
			}

			void lock() {
				if (!mutex) {
					printf("Can't lock empty mutex!");
					for (;;)
						asm("hlt");
				}

				mutex->lock();
			}

			void unlock() {
				if (!mutex) {
					printf("Can't unlock empty mutex!");
					for (;;)
						asm("hlt");
				}

				mutex->try_unlock();
			}

			void release() {
				mutex = nullptr;
			}

			inline operator bool() const {
				return mutex != nullptr;
			}

		private:
			M *mutex = nullptr;
	};
}
