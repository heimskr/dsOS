#pragma once

#include <functional>
#include <list>
#include <map>

#include "Assert.h"

namespace Thorn {
	template <typename K, typename V, template <typename...> typename M, typename E>
	class Cache {
		private:
			size_t maxSize{};

			// The void pointer here is actually a list iterator...
			// I'm sorry.
			M<K, std::pair<void *, V>, E> map;
			using MapIterator = decltype(map)::iterator;

			std::list<MapIterator> list;
			using ListIterator = decltype(list)::iterator;

			static_assert(sizeof(void *) == sizeof(ListIterator));

			void ensureSpace(size_t count) {
				while (map.size() + count > maxSize) {
					map.erase(list.back());
					list.pop_back();
				}
			}

			void moveFront(auto map_iter) {
				auto &list_iter = *reinterpret_cast<ListIterator *>(&map_iter->second.first);
				list.erase(list_iter);
				list.push_front(map_iter);
				list_iter = list.begin();
			}

		public:
			using Key = K;
			using Value = V;

			Cache(size_t max_size): maxSize(max_size) {
				assert(max_size != 0);
			}

			std::pair<MapIterator, bool> emplace(const K &key, V value) {
				if (auto iter = map.find(key); iter != map.end()) {
					moveFront(iter);
					iter->second.second = std::move(value);
					return {iter, false};
				}

				ensureSpace(1);
				auto [iter, inserted] = map.try_emplace(key, nullptr, std::move(value));
				assert(inserted);
				list.push_front(iter);
				iter->second.first = std::bit_cast<void *>(list.begin());
				return {iter, true};
			}

			V & at(const K &key) {
				if (auto iter = map.find(key); iter != map.end()) {
					moveFront(iter);
					return iter->second.second;
				}

				assert(!"Key is present in Cache");
			}

			const V & at(const K &key) const {
				return map.at(key);
			}

			/** The bool in the returned pair indicates whether a new entry was created. */
			std::pair<std::reference_wrapper<V>, bool> operator[](const K &key) {
				if (auto iter = map.find(key); iter != map.end()) {
					moveFront(iter);
					return {std::ref(iter->second.second), false};
				}

				auto [iter, inserted] = emplace(key, V());
				return {std::ref(iter->second.second), true};
			}

			void erase(const K &key) {
				auto iter = map.find(key);
				if (iter == map.end())
					return;

				list.erase(iter->second.first);
				map.erase(iter);
			}

			auto begin() { return map.begin(); }
			auto end() { return map.end(); }
			auto begin() const { return map.begin(); }
			auto end() const { return map.end(); }
	};

	template <typename K, typename V, typename C = std::less<K>>
	using TreeCache = Cache<K, V, std::map, C>;

	template <typename K, typename V, typename H = std::hash<K>>
	using HashCache = Cache<K, V, std::unordered_map, H>;
}
