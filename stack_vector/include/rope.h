#pragma once
#include <list>
#include <vector>
#include "stack_vector.h"

namespace rope {
	template <typename T> struct rope {
	  private:
		std::list<std::vector<T>> _internal_struct;
		size_t                    _size;
		size_t                    _capacity;
	  public:
		using value_type      = T;
		using allocator_type  = std::list<std::vector<T>>::allocator_type;
		using size_type       = std::list<std::vector<T>>::size_type;
		using difference_type = std::list<std::vector<T>>::difference_type;
		using reference       = std::vector<T>::reference;
		using const_reference = std::list<std::vector<T>>::const_reference;
		using pointer         = std::list<std::vector<T>>::pointer;
		using const_pointer   = std::list<std::vector<T>>::const_pointer;
		using iterator        = std::list<std::vector<T>>::iterator;
		using const_iterator  = std::list<std::vector<T>>::const_iterator;
		using reverse_iterator        = std::list<std::vector<T>>::reverse_iterator;
		using const_reverse_iterator  = std::list<std::vector<T>>::const_reverse_iterator;

		constexpr reference front() {
			return _internal_struct[0].front();
		}

		constexpr const_reference front() const {
			return _internal_struct[0].front();
		}

		constexpr reference back() {
			return _internal_struct.back().back();
		}

		constexpr const_reference back() const {
			return _internal_struct.back().back();
		}

		constexpr reference operator[](size_type pos) {
			assert(pos < _size && "index out of bounds");
			for (auto &vect : _internal_struct) {
				if (pos < vect.size()) {
					return vect[pos];
				}
				pos -= vect.size();
			}
			return {};
		}

		constexpr const_reference operator[](size_type pos) const {
			assert(pos < _size && "index out of bounds");
			for (auto &vect : _internal_struct) {
				if (pos < vect.size()) {
					return vect[pos];
				}
				pos -= vect.size();
			}
			return {};
		}

		constexpr size_t size() const {
			return _size;
		}

		constexpr size_t capacity() const {
			return _capacity;
		}

		// add a new vector to the end to expand on
		__forceinline constexpr void grow() {
			auto &new_vec = _internal_struct.emplace_back();
			new_vec.reserve(_capacity ? _capacity * 2 : 8);
			_capacity += new_vec.capacity();
		}
		
		template <class... Args>
		constexpr reference emplace_back(Args &&...args) {
			if (!_internal_struct.empty()) {
				auto &last_struct = _internal_struct.back();
				if (last_struct.size() == last_struct.capacity()) {
					auto &new_vec = _internal_struct.emplace_back();
					new_vec.reserve(_capacity ? _capacity * 2 : 8);
					_capacity += new_vec.capacity();
					new_vec.emplace_back(std::forward<Args>(args)...);
				} else {
					// technically shouldn't reallocate
					_capacity -= last_struct.capacity();
					_internal_struct.back().emplace_back(std::forward<Args>(args)...);
					_capacity += last_struct.capacity();
				}

			} else {
				auto &new_vec = _internal_struct.emplace_back();
				new_vec.reserve(_capacity ? _capacity * 2 : 8);
				_capacity += new_vec.capacity();
				new_vec.back().emplace_back(std::forward<Args>(args)...);
			}
			_size += 1;
		};

		template <class... Args> constexpr const_reference emplace_back(Args &&...args) const {
			if (!_internal_struct.empty()) {
				auto &last_struct = _internal_struct.back();
				if (last_struct.size() == last_struct.capacity()) {
					auto &new_vec = _internal_struct.emplace_back();
					new_vec.reserve(_capacity ? _capacity * 2 : 8);
					_capacity += new_vec.capacity();
					new_vec.emplace_back(std::forward<Args>(args)...);
				} else {
					// technically shouldn't reallocate
					_capacity -= last_struct.capacity();
					_internal_struct.back().emplace_back(std::forward<Args>(args)...);
					_capacity += last_struct.capacity();
				}
			} else {
				auto &new_vec = _internal_struct.emplace_back();
				new_vec.reserve(_capacity ? _capacity * 2 : 8);
				_capacity += new_vec.capacity();
				new_vec.back().emplace_back(std::forward<Args>(args)...);
			}
			_size += 1;
		};
	};

	template <typename T, size_t rope_width> struct block_rope { private:
		std::list<stack_vector::stack_vector<T, rope_width>> _internal_struct;
		size_t                                               _size;
		size_t                                               _capacity;
	  public:
		using value_type      = T;
		using allocator_type  = std::list<std::vector<T>>::allocator_type;
		using size_type       = std::list<std::vector<T>>::size_type;
		using difference_type = std::list<std::vector<T>>::difference_type;
		using reference       = std::vector<T>::reference;
		using const_reference = std::list<std::vector<T>>::const_reference;
		using pointer         = std::list<std::vector<T>>::pointer;
		using const_pointer   = std::list<std::vector<T>>::const_pointer;
		using iterator        = std::list<std::vector<T>>::iterator;
		using const_iterator  = std::list<std::vector<T>>::const_iterator;
		using reverse_iterator        = std::list<std::vector<T>>::reverse_iterator;
		using const_reverse_iterator  = std::list<std::vector<T>>::const_reverse_iterator;

		constexpr reference front() {
			return _internal_struct[0].front();
		}

		constexpr const_reference front() const {
			return _internal_struct[0].front();
		}

		constexpr reference back() {
			return _internal_struct.back().back();
		}

		constexpr const_reference back() const {
			return _internal_struct.back().back();
		}

		constexpr reference operator[](size_type pos) {
			assert(pos < _size && "index out of bounds");
			size_t value_idx = pos % rope_width;
			size_t rope_idx  = pos / rope_width;
			return _internal_struct[rope_idx][value_idx];
		}

		constexpr const_reference operator[](size_type pos) const {
			assert(pos < _size && "index out of bounds");
			size_t value_idx = pos % rope_width;
			size_t rope_idx  = pos / rope_width;
			return _internal_struct[rope_idx][value_idx];
		}

		constexpr size_t size() const {
			return _size;
		}

		constexpr size_t capacity() const {
			return _capacity;
		}

		template <class... Args> constexpr reference emplace_back(Args &&...args) {
			if (!_internal_struct.empty()) {
				auto &last_struct = _internal_struct.back();
				if (last_struct.size() == last_struct.capacity()) {
					auto &new_vec = _internal_struct.emplace_back();
					new_vec.reserve(_capacity ? _capacity * 2 : 8);
					_capacity += new_vec.capacity();
					new_vec.emplace_back(std::forward<Args>(args)...);
				} else {
					// technically shouldn't reallocate
					_capacity -= last_struct.capacity();
					_internal_struct.back().emplace_back(std::forward<Args>(args)...);
					_capacity += last_struct.capacity();
				}

			} else {
				auto &new_vec = _internal_struct.emplace_back();
				new_vec.reserve(_capacity ? _capacity * 2 : 8);
				_capacity += new_vec.capacity();
				new_vec.back().emplace_back(std::forward<Args>(args)...);
			}
			_size += 1;
		};

		template <class... Args> constexpr const_reference emplace_back(Args &&...args) const {
			if (!_internal_struct.empty()) {
				auto &last_struct = _internal_struct.back();
				if (last_struct.size() == last_struct.capacity()) {
					auto &new_vec = _internal_struct.emplace_back();
					new_vec.reserve(_capacity ? _capacity * 2 : 8);
					_capacity += new_vec.capacity();
					new_vec.emplace_back(std::forward<Args>(args)...);
				} else {
					// technically shouldn't reallocate
					_capacity -= last_struct.capacity();
					_internal_struct.back().emplace_back(std::forward<Args>(args)...);
					_capacity += last_struct.capacity();
				}
			} else {
				auto &new_vec = _internal_struct.emplace_back();
				new_vec.reserve(_capacity ? _capacity * 2 : 8);
				_capacity += new_vec.capacity();
				new_vec.back().emplace_back(std::forward<Args>(args)...);
			}
			_size += 1;
		};
	}
} // namespace rope
