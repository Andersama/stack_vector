#pragma once
#include <array>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>

#include <assert.h>

namespace stack_vector {
    namespace details {
        // std::array<T, N> store;
        template <typename _Ty> constexpr void destroy_at(_Ty *const ptr) {
            ptr->~_Ty();
        }
        template <typename It1> constexpr void destroy(It1 first, It1 last) {
            for (; first != last; ++first)
                ::stack_vector::details::destroy_at(::std::addressof(*first));
        }

        template <typename It1, typename It2> constexpr void uninitialized_copy(It1 I, It1 E, It2 Dest) {
            ::std::uninitialized_copy(I, E, Dest);
        }
        template <typename It1, typename It2> constexpr void uninitialized_copy_n(It1 I, size_t C, It2 Dest) {
            ::std::uninitialized_copy_n(I, C, Dest);
        }
        template <typename It1, typename It2> constexpr void uninitialized_move(It1 I, It1 E, It2 Dest) {
            ::std::uninitialized_copy(::std::make_move_iterator(I), ::std::make_move_iterator(E), Dest);
        }
        template <typename It1, typename It2> constexpr void uninitialized_move_n(It1 I, size_t C, It2 Dest) {
            ::std::uninitialized_copy_n(::std::make_move_iterator(I), C, Dest);
        }
        template <typename It1, typename Val1> constexpr void uninitialized_fill(It1 I, It1 E, Val1 Dest) {
            ::std::uninitialized_fill(I, E, Dest);
        }
        template <typename It1, typename Val1> constexpr void uninitialized_fill_n(It1 I, size_t C, Val1 V) {
            ::std::uninitialized_fill_n(I, C, V);
        }
    }; // namespace details

    template <typename T, size_t N> struct stack_vector {
        static_assert(N > 0, "a stack_vector<T,N> must have an N > 0");
      public:
        using element_type           = T;
        using value_type             = typename ::std::remove_cv<T>::type;
        using const_reference        = const value_type &;
        using size_type              = ::std::size_t;
        using difference_type        = ::std::ptrdiff_t;
        using pointer                = element_type *;
        using const_pointer          = const element_type *;
        using reference              = element_type &;
        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = ::std::reverse_iterator<iterator>;
        using const_reverse_iterator = ::std::reverse_iterator<const_iterator>;

      private:
        using array_type = ::std::array<T, N>;
        using union_type = ::std::aligned_union<1, array_type>;
        size_type _size  = 0ULL;
        // Allocate raw space for N elements of type T.  If T has a ctor or dtor, we
        // don't want it to be automatically run, so we need to represent the space as
        // something else.  Use an array of char of sufficient alignment.
        union {
            char             a_byte;
            T                a_t;
            std::array<T, N> store;
        };

        template <class It1> void insert_range(const_iterator pos, It1 first, It1 last) {
            // insert input range [first, last) at _Where
            if (first == last) {
                return; // nothing to do, avoid invalidating iterators
            }

            pointer         old_begin  = &store[0];
            pointer         oldlast    = &store[0] + size();
            const size_type insert_idx = pos - cbegin();
            const size_type old_size   = size();
            // For one-at-back, provide strong guarantee.
            // Otherwise, provide basic guarantee (despite N4659 26.3.11.5 [vector.modifiers]/1).
            // Performance note: except for one-at-back, emplace_back()'s strong guarantee is unnecessary
            // here.

            for (; first != last && size() < capacity(); ++first) {
                unchecked_emplace_back(*first); // emplace_back(*first);
            }

            ::std::rotate(begin() + insert_idx, begin() + old_size, end());
        }

        template <class It1> void append_range(It1 first, It1 last) {
            if (first == last) {
                return; // nothing to do, avoid invalidating iterators
            }
            // For one-at-back, provide strong guarantee.
            // Otherwise, provide basic guarantee (despite N4659 26.3.11.5 [vector.modifiers]/1).
            // Performance note: except for one-at-back, emplace_back()'s strong guarantee is unnecessary
            // here.
            for (; first != last && size() < capacity(); ++first) {
                unchecked_emplace_back(*first); // emplace_back(*first);
            }
        }

      public:
        // constructor's
        constexpr stack_vector() noexcept {
        }
        constexpr stack_vector(size_type count, const T &value) {
            assign(count, value);
        }
        constexpr explicit stack_vector(size_type count) {
            assign(count, T());
        }

        template <class It1> stack_vector(It1 first, It1 last) {
            append(first, last);
        }

        constexpr stack_vector(const stack_vector &other) {
            if (!other.empty())
                this->operator=(other);
        };
        constexpr stack_vector(stack_vector &&other) noexcept {
            if (!other.empty())
                this->operator=(::std::move(other));
        };
        constexpr stack_vector(::std::initializer_list<T> init) {
            assign(init);
        };
        template <size_t N0> constexpr stack_vector(std::array<T, N0> arr) noexcept {
            if (!arr.empty()) {
                assign(arr.begin(), arr.end());
            }
        }

        // destructor
        ~stack_vector() {
            if constexpr (!::std::is_pod<element_type>::value)
                ::stack_vector::details::destroy(begin(), end());
        }
        // operator ='s
        constexpr stack_vector &operator=(const stack_vector &other) {
            if (this == &other)
                return *this;
            size_t rhs_size = other.size();
            size_t lhs_size = size();
            if (lhs_size >= rhs_size) {
                iterator new_end;
                if (rhs_size)
                    new_end = ::std::copy(other.begin(), other.begin() + rhs_size, begin());
                else // no copy, setup to clear
                    new_end = begin();
                // destroy excess elements
                ::stack_vector::details::destroy(new_end, end());
                _size = rhs_size;
                return *this;
            } else {
                // copy to initialized memory
                ::std::copy(other.begin(), other.begin() + lhs_size, begin());
                // copy to uninitialized memory
                ::stack_vector::details::uninitialized_copy(other.begin() + lhs_size,
                                                            other.begin() + rhs_size, begin() + lhs_size);
                _size = rhs_size;
                return *this;
            }
        };
        constexpr stack_vector &operator=(stack_vector &&other) noexcept {
            size_t rhs_size = other.size();
            size_t lhs_size = size();
            if (this == &other) {
                // do nothing
            } else if (lhs_size >= rhs_size) {
                // assign
                iterator new_end;
                if (rhs_size)
                    new_end = ::std::move(other.begin(), other.begin() + rhs_size, begin());
                else // no copy, setup to clear
                    new_end = begin();
                // destroy excess elements
                ::stack_vector::details::destroy(new_end, end());
                _size = rhs_size;
                // clear other
                other.clear();
            } else {
                // copy to initialized memory
                ::std::move(other.begin(), other.begin() + lhs_size, begin());
                // copy to uninitialized memory
                ::stack_vector::details::uninitialized_move(other.begin() + lhs_size,
                                                            other.begin() + rhs_size, begin() + lhs_size);
                _size = rhs_size;
                other.clear();
            }
            return *this;
        };
        constexpr stack_vector &operator=(::std::initializer_list<T> ilist) {
            assign(ilist);
            return *this;
        };
        // assign's
        constexpr void assign(size_type count, const T &value) {
            if (count < capacity())
                [[likely]] {
                    clear();
                    ::stack_vector::details::uninitialized_fill_n(end(), count, value);
                    _size = count;
                }
        };
        template <class It1> constexpr void assign(It1 first, It1 last) {
            size_t insert_count = ::std::distance(first, last);
            if ((size() + insert_count) <= capacity()) {
                clear();
                ::stack_vector::details::uninitialized_copy_n(first, insert_count, end());
                _size = insert_count;
            }
        };
        constexpr void assign(::std::initializer_list<T> ilist) {
            assign(ilist.begin(), ilist.end());
        };
        // append's (non-standard)
        void append(size_type count, const T &value) {
            size_t space_remaining = capacity() - size();
            if (count <= space_remaining) {
                ::stack_vector::details::uninitialized_fill_n(end(), count, value);
                _size += count;
            }
        }
        template <typename It1> void append(It1 first, It1 last) {
            if constexpr (::std::is_same<::std::random_access_iterator_tag,
                                         ::std::iterator_traits<It1>::iterator_category>::value) {
                size_t insert_count = ::std::distance(first, last);
                if ((size() + insert_count) <= capacity()) {
                    ::stack_vector::details::uninitialized_copy(first, last, end());
                    _size += insert_count;
                }
            } else {
                append_range(first, last);
            }
        }

        // at's
        [[nodiscard]] constexpr reference at(size_type pos) {
            return store.at(pos);
        };
        [[nodiscard]] constexpr const_reference at(size_type pos) const {
            return store.at(pos);
        };
        //[]'s
        [[nodiscard]] constexpr reference operator[](size_type pos) {
            assert(pos < size());
            return store[pos];
        };
        [[nodiscard]] constexpr const_reference operator[](size_type pos) const {
            assert(pos < size());
            return store[pos];
        };
        // front
        [[nodiscard]] constexpr reference front() {
            assert(!empty());
            return store[0];
        };
        [[nodiscard]] constexpr const_reference front() const {
            assert(!empty());
            return store[0];
        };
        // back
        [[nodiscard]] constexpr reference back() {
            assert(!empty());
            return store[_size - 1];
        };
        [[nodiscard]] constexpr const_reference back() const {
            assert(!empty());
            return store[_size - 1];
        };
        // data
        [[nodiscard]] constexpr T *data() noexcept {
            return store.data();
        };
        [[nodiscard]] constexpr const T *data() const noexcept {
            return store.data();
        };
        // begin's
        [[nodiscard]] constexpr iterator begin() noexcept {
            return &store[0];
        };
        [[nodiscard]] constexpr const_iterator begin() const noexcept {
            return &store[0];
        };
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
            return &store[0];
        };
        // rbegin's
        [[nodiscard]] constexpr reverse_iterator rbegin() noexcept {
            return reverse_iterator(end());
        };
        [[nodiscard]] constexpr const_reverse_iterator rbegin() const noexcept {
            return const_reverse_iterator(end());
        };
        [[nodiscard]] constexpr const_reverse_iterator crbegin() const noexcept {
            return const_reverse_iterator(end());
        };
        // end's
        [[nodiscard]] constexpr iterator end() noexcept {
            return &store[0] + size();
        };
        [[nodiscard]] constexpr const_iterator end() const noexcept {
            return &store[0] + size();
        };
        [[nodiscard]] constexpr const_iterator cend() const noexcept {
            return &store[0] + size();
        };
        // rend's
        [[nodiscard]] constexpr reverse_iterator rend() noexcept {
            return reverse_iterator(begin());
        };
        [[nodiscard]] constexpr const_reverse_iterator rend() const noexcept {
            return const_reverse_iterator(begin());
        };
        [[nodiscard]] constexpr const_reverse_iterator crend() const noexcept {
            return const_reverse_iterator(begin());
        };
        // empty
        [[nodiscard]] constexpr bool empty() const noexcept {
            return size() == 0;
        };
        // full (non standard)
        [[nodiscard]] constexpr bool full() const noexcept {
            return size() >= capacity();
        };
        // size
        constexpr size_type size() const noexcept {
            return _size;
        };
        // capacity (bit of a lie, we forward to array's size)
        constexpr size_type capacity() const noexcept {
            return N;
        };
        // max_size (bit of a lie, we forward to array's size)
        constexpr size_type max_size() const noexcept {
            return N;
        };
        // reserve (unimplemented or noop)
        // shrink_to_fit (unimplemented or noop)
        // ::std::uninitialized_fill(this->begin(), this->end(), Elt);
        constexpr void clear() noexcept {
            if constexpr (::std::is_pod<element_type>::value) {
                _size = 0ULL;
            } else {
                ::stack_vector::details::destroy(begin(), end());
                _size = 0ULL;
            }
        };
        // insert's TODO!
        // be sure to verify range
        constexpr iterator insert(const_iterator pos, const T &value) {
            return emplace(pos, value);
        };
        constexpr iterator insert(const_iterator pos, T &&value) {
            return emplace(pos, ::std::move(value));
        };
        constexpr iterator insert(const_iterator pos, size_type count, const T &value) {
            const pointer insert_ptr = &(*pos);
            const pointer old_begin  = &(*begin());
            const pointer old_end    = &(*end());
            const pointer new_end    = old_end + count;
            const T *     value_ptr  = &value;

            size_type  insert_idx         = pos - cend();
            size_type  remaining_capacity = capacity() - size();
            const bool one_at_back        = (count == 1) && (insert_ptr == old_end);
            if (count == 0) {                        // do nothing
            } else if (count > remaining_capacity) { // error?
            } else if (one_at_back) {
                unchecked_emplace_back(value);
            } else {
                T tmp = value;
                std::move_backward(insert_ptr, old_end, old_end + count);
                ::stack_vector::details::uninitialized_fill_n(pos, count, tmp); // ok?
                _size += count;
            }
            return iterator(insert_ptr);
        };
        template <class InputIt> constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
            size_type insert_idx = pos - cbegin();
            insert_range(pos, first, last);
            return begin() + insert_idx;
        };
        constexpr iterator insert(const_iterator pos, ::std::initializer_list<T> ilist) {
            return insert(pos, ilist.begin(), ilist.end());
        };
        // emplace's
        template <class... Args> constexpr iterator emplace(const_iterator pos, Args &&... args) {
            size_type insert_idx = pos - cbegin();
            iterator  ret_idx    = begin() + insert_idx;
            if (pos == cend()) { // special case for empty vector
                if (!full()) {
                    ::new ((void *)ret_idx) T(::std::forward<Args>(args)...);
                    _size += 1;
                    return ret_idx;
                } else { // error?
                    return ret_idx;
                }
            }

            assert(pos >= cbegin() && "insertion iterator is out of bounds.");
            assert(pos <= cend() && "inserting past the end of the stack_vector.");
            // if full we back out
            if (full()) {
                return ret_idx = end();
            }
            T tmp = T(::std::forward<Args>(args)...);
            // placement new back item, eg... ...insert here, a, b, c, end -> ...insert here, a, a, b, c,
            // (end)
            ::new ((void *)end()) T(::std::move(back()));
            // first last, destination
            ::std::move_backward(begin() + insert_idx, end() - 1, end());
            // up the size
            _size += 1;
            // insert the value
            *ret_idx = ::std::move(tmp);

            return ret_idx;
        };

        // push_back's
        constexpr void push_back(const T &value) {
            emplace_back(::std::forward<const T &>(value));
        }
        constexpr void push_back(T &&value) {
            emplace_back(::std::forward<T &&>(value));
        };
        // shove_back's (unchecked_push_back)
        constexpr void shove_back(const T &value) {
            ::new ((void *)end()) T(::std::forward<const T &>(value));
            _size += 1;
        }
        constexpr void shove_back(T &&value) {
            ::new ((void *)end()) T(::std::forward<T &&>(value));
            _size += 1;
        }
        // emplace_back's
        template <class... Args> constexpr reference emplace_back(Args &&... args) {
            iterator it = end();
            if (size() < capacity())
                [[likely]] {
                    ::new ((void *)it) T(::std::forward<Args>(args)...);
                    _size += 1;
                }
            return *it;
        };
        template <class... Args> constexpr reference unchecked_emplace_back(Args &&... args) {
            iterator it = end();
            ::new ((void *)it) T(::std::forward<Args>(args)...);
            _size += 1;
            return *it;
        };

        // pop_back's
        constexpr void pop_back() {
            if (size())
                [[likely]] {
                    if constexpr (::std::is_pod<element_type>::value) {
                        _size -= 1;
                    } else {
                        _size -= 1;
                        end()->~T(); // destroy the tailing value
                    }
                }
        };
        // resize's (unimplemented or noop)
        // swap's
        constexpr void swap(stack_vector &other) noexcept {
            if (this == &other)
                return;
            if constexpr (::std::is_pod<T>::value) {
                // swap the data
                ::std::swap(*reinterpret_cast<array_type *>(&store),
                            *reinterpret_cast<array_type *>(&other.store));
            } else {
                stack_vector tmp = *this; // meh
                assign(other);
                other.assign(tmp);
            }
            // swap the sizes
            ::std::swap(_size, other._size);
        }
    };
} // namespace stack_vector