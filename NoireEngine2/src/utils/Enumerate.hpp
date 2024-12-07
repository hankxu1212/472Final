#pragma once

#include <iterator>
#include <tuple>

/**
  * C++ implementation of Python's Enumerate function
  * 
  * \param iterable the container to iterate on
  * \return [&idx, &obj]
*/
template<typename T,
	typename TIter = decltype(std::begin(std::declval<T>())),
	typename = decltype(std::end(std::declval<T>()))>
constexpr auto Enumerate(T&& iterable) {
	struct iterator {
		size_t i;
		TIter iter;
		bool operator!=(const iterator& rhs) const { return iter != rhs.iter; }
		void operator++() { ++i; ++iter; }
		auto operator*() const { return std::tie(i, *iter); }
	};
	struct iterable_wrapper {
		T iterable;
		auto begin() { return iterator{ 0, std::begin(iterable) }; }
		auto end() { return iterator{ 0, std::end(iterable) }; }
	};
	return iterable_wrapper{ std::forward<T>(iterable) };
}
