#pragma once

template <typename T>
void swap(T &a, T &b) {
	T copy = a;
	a = b;
	b = copy;
}
