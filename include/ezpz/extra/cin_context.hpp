#pragma once
#include "ezpz/context.hpp"
#include <iostream>
#include <vector>
#include <ranges>

namespace ezpz {

	template<std::ranges::input_range I_RANGE>
	struct make_bi_range {
		using RANGE = make_bi_range<I_RANGE>;
		using I_ITER = typename I_RANGE::iterator;
		using value_type = typename I_RANGE::value_type;
		using I_END_ITER  = decltype(std::end(std::declval<I_RANGE>()));

		I_RANGE parent;
		mutable I_ITER iter;
		mutable std::vector<value_type> cache;

		explicit make_bi_range(I_RANGE p)
			: parent(p)
			, iter(std::begin(p))
		{}

		struct iterator {
			using difference_type = int32_t;
			using value_type = typename std::iterator_traits<I_ITER>::value_type;
			using reference = value_type&;
			using pointer = value_type*;
			using iterator_category = std::random_access_iterator_tag;

			const RANGE* parent = nullptr;
			size_t index = 0;

			iterator() = default;
			iterator(iterator&&) = default;
			iterator(const iterator&) = default;
			iterator(const RANGE* p, size_t i) 
				: parent(p)
				, index(i)
			{}
			iterator& operator=(iterator&&) = default;
			iterator& operator=(const iterator&) = default;
			
			value_type operator*() const {
				return parent->cache[index];
			}
			bool operator==(const I_END_ITER& other) const {
				return index == parent->cache.size() && parent->iter == other;
			}
			bool operator==(const iterator& other) const {
				return index == other.index && parent == other.parent;
			}
			bool operator!=(const iterator& other) const {
				return index != other.index && parent != other.parent;
			}
			iterator operator--(int) {
				auto ret = *this;
				--ret;
				return ret;
			}
			iterator& operator--() {
				--index;
				return *this;
			}
			iterator operator++(int) {
				auto ret = *this;
				++ret;
				return ret;
			}
			iterator& operator++() {
				++index;
				parent->assure(index);
				return *this;
			}
		};

		static_assert(std::bidirectional_iterator<iterator>);

		void assure(size_t index) const {
			while(iter != std::end(parent) && cache.size() <= index){
				cache.emplace_back(*iter);
				++iter;
			}
		}
		iterator begin() const {
			assure(0);
			return iterator{this,0};
		}
		auto end() const {
			return std::end(parent);
		}

	};

	struct cin_range {
		using iterator = std::istream_iterator<char>;
		using value_type = char;

		auto begin() const {
			return std::istream_iterator<char>(std::cin);
		}
		auto end() const {
			return std::istream_iterator<char>();
		}
	};
	static_assert(std::ranges::input_range<cin_range>);

	/* inline auto cin_context = */ 
	auto make_cin_context(){
		return forward_range_context<make_bi_range<cin_range>>{make_bi_range<cin_range>{cin_range{}}};
	}
	/* template<std::ranges::input_range R> */ 
	/* struct input_range_context : input_range_context_iterator { */

	/* } */
}
