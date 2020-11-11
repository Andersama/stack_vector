#pragma once
#include <algorithm>
#include <array>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <assert.h>

namespace stack_vector {
    // for some detail trickery
    // see: https://github.com/tcbrindle/span/blob/master/include/tcb/span.hpp
    namespace details {
        template <typename _Ty> constexpr void destroy_at(_Ty *const ptr) {
            ptr->~_Ty();
        }
        template <typename It1> constexpr void destroy(It1 first, It1 last) {
            for (; first != last; ++first)
                ::stack_vector::details::destroy_at(::std::addressof(*first));
        }

        template <typename It1, typename It2> constexpr It1 uninitialized_copy(It1 I, It1 E, It2 Dest) {
            return ::std::uninitialized_copy(I, E, Dest);
        }
        template <typename It1, typename It2> constexpr It1 uninitialized_copy_n(It1 I, size_t C, It2 Dest) {
            return ::std::uninitialized_copy_n(I, C, Dest);
        }
        template <typename It1, typename It2> constexpr It1 uninitialized_move(It1 I, It1 E, It2 Dest) {
            return ::std::uninitialized_copy(::std::make_move_iterator(I), ::std::make_move_iterator(E),
                                             Dest);
        }
        template <typename It1, typename It2> constexpr It1 uninitialized_move_n(It1 I, size_t C, It2 Dest) {
            return ::std::uninitialized_copy_n(::std::make_move_iterator(I), C, Dest);
        }
        template <typename It1, typename Val1> constexpr void uninitialized_fill(It1 I, It1 E, Val1 Dest) {
            ::std::uninitialized_fill(I, E, Dest);
        }
        template <typename It1, typename Val1> constexpr void uninitialized_fill_n(It1 I, size_t C, Val1 V) {
            ::std::uninitialized_fill_n(I, C, V);
        }

        enum class error_handling : uint8_t { _noop, _saturate, _exception, _error_code };
        constexpr const error_handling error_handler = error_handling::_noop;
    }; // namespace details

    template <typename T> struct alignas(alignof(T)) non_trivial_struct {
        constexpr non_trivial_struct() noexcept {};
        constexpr T *as_ptr() noexcept {
            return reinterpret_cast<T *>(this);
        }
        constexpr const T *as_ptr() const noexcept {
            return reinterpret_cast<const T *>(this);
        }
    };

    template <typename T, size_t N, bool = ::std::is_trivially_destructible<T>::value>
    struct stack_vector_destruct_base {
        using value_type = typename ::std::remove_const_t<T>;
        union {
            non_trivial_struct<value_type> _dummy;
            value_type                     _buf[N];
            //::std::array<value_type, N>    _buf;
        };
        uint32_t _size;
        uint32_t _more_inited;
        // uint8_t  _using_buf;
        constexpr stack_vector_destruct_base() noexcept : _dummy{}, _size{0}, _more_inited{0} {};
    };

    template <typename T, size_t N> struct stack_vector_destruct_base<T, N, false> {
        using value_type = ::std::remove_const_t<T>;
        union {
            non_trivial_struct<value_type> _dummy;
            value_type                     _buf[N];
            //::std::array<value_type, N>    _buf;
        };
        uint32_t _size;
        uint32_t _more_inited;
        // uint32_t _using_buf;
        constexpr stack_vector_destruct_base() noexcept : _dummy{}, _size{0}, _more_inited{0} {};
        ~stack_vector_destruct_base() noexcept {
            if constexpr (::std::is_constant_evaluated()) {
                // we have to keep track of which union member we're dealing w/ (b/c otherwise the compiler
                // complains) :(
                if (_size || _more_inited) {
                    // clamp the thing we're iterating over
                    size_t count =
                        (this->_size + this->_more_inited) < N ? (this->_size + this->_more_inited) : N;
                    for (size_t i = 0; i < count; i++)
                        _buf[i].~value_type();
                }
            } else {
                size_t count =
                    (this->_size + this->_more_inited) < N ? (this->_size + this->_more_inited) : N;
                for (size_t i = 0; i < count; i++)
                    _buf[i].~value_type();
            }
        }
    };

    template <class T, size_t N> struct stack_vector_construct_base : stack_vector_destruct_base<T, N> {
        using stack_vector_destruct_base<T, N>::stack_vector_destruct_base;
    };

    template <typename T, size_t N> struct stack_vector : private stack_vector_construct_base<T, N> {
        static_assert(N > 0, "a stack_vector<T,N> must have an N > 0");

      public:
        using destruct_base  = stack_vector_destruct_base<T, N>;
        using construct_base = stack_vector_construct_base<T, N>;
        using value_type     = typename stack_vector_destruct_base<T, N>::value_type;
        using array_type     = value_type[N];
        //::std::array<value_type, N>;
        using const_reference        = const value_type &;
        using size_type              = ::std::size_t;
        using difference_type        = ::std::ptrdiff_t;
        using pointer                = value_type *;
        using const_pointer          = const value_type *;
        using reference              = value_type &;
        using iterator               = pointer;
        using const_iterator         = const_pointer;
        using reverse_iterator       = ::std::reverse_iterator<iterator>;
        using const_reverse_iterator = ::std::reverse_iterator<const_iterator>;

      private:
        template <typename RetType>
        constexpr __forceinline RetType return_error(RetType ret, [[maybe_unused]] const char *err_msg) {
            if constexpr (::stack_vector::details::error_handler ==
                          ::stack_vector::details::error_handling::_noop) {
                return ret;
            } else if (::stack_vector::details::error_handler ==
                       ::stack_vector::details::error_handling::_saturate) {
                return ret;
            } else if (::stack_vector::details::error_handler ==
                       ::stack_vector::details::error_handling::_exception) {
                throw std::bad_alloc(err_msg);
                return ret;
            } else if (::stack_vector::details::error_handler ==
                       ::stack_vector::details::error_handling::_error_code) {
                if constexpr (std::is_same<RetType, iterator>::value) {
                    return ++ret;
                } else if (std::is_same<RetType, bool>::value) {
                    return false;
                }
            } else {
                return ret;
            }
        };
        // done
        template <typename It1, typename It2>
        constexpr iterator insert_range(const_iterator pos, It1 first, It2 last) {
            // insert input range [first, last) at _Where
            if (::std::is_constant_evaluated()) {
                size_type insert_idx = pos - cbegin();
                iterator  ret_it     = begin() + insert_idx;
                if (first == last) {
                    return ret_it;
                }
                assert(pos >= cbegin() && pos <= cend() &&
                       "insert iterator is out of bounds of the stack_vector");
                const size_type old_size = size();

                // bounds check each emplace_back, saturating
                for (; first != last && size() < capacity(); ++first) {
                    unchecked_emplace_back(*first);
                }

                if constexpr (::stack_vector::details::error_handler !=
                              ::stack_vector::details::error_handling::_noop) {
                    if (first != last) {
                        return ret_it = return_error(ret_it, "stack_vector cannot allocate space to insert");
                    }
                }

                ::std::rotate(begin() + insert_idx, begin() + old_size, end());
                return ret_it;
            } else {
                size_type insert_idx = pos - cbegin();
                iterator  ret_it     = begin() + insert_idx;
                if (first == last) {
                    return ret_it;
                }
                assert(pos >= cbegin() && pos <= cend() &&
                       "insert iterator is out of bounds of the stack_vector");
                const size_type old_size = size();

                if constexpr (::std::is_same<::std::random_access_iterator_tag,
                                             ::std::iterator_traits<It1>::iterator_category>::value) {
                    size_type insert_count = last - first;
                    if (insert_count > (capacity() - size())) { // error? or noop
                        if constexpr (::stack_vector::details::error_handler !=
                                      ::stack_vector::details::error_handling::_noop) {
                            return ret_it =
                                       return_error(ret_it, "stack_vector cannot allocate space to insert");
                        }
                    }
                    size_t copy_count        = insert_count > init_size() ? init_size() : insert_count;
                    size_t uninit_copy_count = insert_count - copy_count;
                    // already safe from check above*
                    auto last_copied = ::std::copy_n(first, copy_count, end());
                    ::std::uninitialized_copy_n(first + copy_count, uninit_copy_count, last_copied);
                    if constexpr (::std::is_trivially_destructible<value_type>::value) {
                        this->_more_inited -= copy_count;
                    }
                    //::stack_vector::details::uninitialized_copy_n(first, insert_count, end());
                    this->_size += insert_count;
                } else {
                    // bounds check each emplace_back, saturating
                    for (; first != last && size() < capacity(); ++first) {
                        unchecked_emplace_back(*first);
                    }

                    if constexpr (::stack_vector::details::error_handler !=
                                  ::stack_vector::details::error_handling::_noop) {
                        if (first != last) {
                            return ret_it =
                                       return_error(ret_it, "stack_vector cannot allocate space to insert");
                        }
                    }
                }

                if constexpr (::stack_vector::details::error_handler !=
                              ::stack_vector::details::error_handling::_noop) {
                    if (first != last) {
                        return ret_it = return_error(ret_it, "stack_vector cannot allocate space to insert");
                    }
                }

                ::std::rotate(begin() + insert_idx, begin() + old_size, end());
                return ret_it;
            }
        }
        // done
        template <typename It1, typename It2> constexpr iterator append_range(It1 first, It2 last) {
            // insert input range [first, last) at _Where
            iterator ret_it = end();
            if (first == last) {
                return ret_it;
            }

            if constexpr (::std::is_same<::std::random_access_iterator_tag,
                                         ::std::iterator_traits<It1>::iterator_category>::value) {
                size_type insert_count = last - first;
                if (insert_count > (capacity() - size())) { // error? or noop
                    if constexpr (::stack_vector::details::error_handler !=
                                  ::stack_vector::details::error_handling::_noop) {
                        return ret_it = return_error(ret_it, "stack_vector cannot allocate space to insert");
                    }
                }
                // already safe from check above*
                if (::std::is_constant_evaluated()) {
                    for (; first != last && size() < capacity(); ++first) {
                        unchecked_emplace_back(*first);
                    }
                } else {
                    ::stack_vector::details::uninitialized_copy_n(first, insert_count, end());
                }
                this->_size += insert_count;
            } else {
                // bounds check each emplace_back, saturating
                for (; first != last && size() < capacity(); ++first) {
                    unchecked_emplace_back(*first);
                }
                if constexpr (::stack_vector::details::error_handler !=
                              ::stack_vector::details::error_handling::_noop) {
                    if (first != last) {
                        return ret_it = return_error(ret_it, "stack_vector cannot allocate space to insert");
                    }
                }
            }

            return ret_it;
        }
        // done
        constexpr void construct(size_type count, const value_type &value) {
            if (::std::is_constant_evaluated()) {
                // default construct remaining b/c otherwise we leak at runtime
                // this->_buf           = array_type();
                size_t       i       = 0;
                const size_t clamped = count <= N ? count : N;
                for (; i < clamped; i++)
                    this->_buf[i] = value;
                for (; i < N; i++)
                    this->_buf[i] = value_type();

                this->_size        = count;
                this->_more_inited = N - count;
            } else {
                size_t clamped = count <= N ? count : N;
                ::std::uninitialized_fill_n(begin(), clamped, value);
                this->_size = count;
            }
        }
        // done
        constexpr void construct(size_type count) {
            if (::std::is_constant_evaluated()) {
                size_t i = 0;
                // this->_buf         = array_type();
                for (; i < N; i++)
                    this->_buf[i] = value_type();
                this->_size        = count;
                this->_more_inited = N - count;
            } else {
                size_t clamped = count <= N ? count : N;
                ::std::uninitialized_default_construct_n(begin(), clamped);
                //::std::uninitialized_fill_n(begin(), count, value);
                this->_size = clamped;
            }
        }
        // done
        template <typename T1, typename T2> constexpr void construct_range(T1 first, T2 last) {
            if (::std::is_constant_evaluated()) {
                if constexpr (::std::is_same<::std::random_access_iterator_tag,
                                             ::std::iterator_traits<T1>::iterator_category>::value) {
                    size_t i     = 0;
                    size_t count = std::distance(first, last);
                    // this->_buf   = array_type();
                    for (; i < N && i < count; ++i) {
                        this->_buf[i] = *(first + i);
                    }
                    for (; i < N; i++)
                        this->_buf[i] = value_type();

                    if constexpr (::stack_vector::details::error_handler ==
                                  ::stack_vector::details::error_handling::_exception) {
                        if (first != last)
                            throw std::bad_alloc("stack_vector cannot allocate in order to construct");
                    }

                    this->_more_inited = N - count;
                    this->_size        = count;
                } else {
                    size_t i = 0;
                    // this->_buf  = array_type();
                    this->_size = 0;
                    for (; i < N && first != last; ++i, ++first) {
                        this->_buf[i] = *first;
                        this->_size += 1;
                    }
                    for (; i < N; i++)
                        this->_buf[i] = value_type();

                    if constexpr (::stack_vector::details::error_handler ==
                                  ::stack_vector::details::error_handling::_exception) {
                        if (first != last)
                            throw std::bad_alloc("stack_vector cannot allocate in order to construct");
                    }

                    this->_more_inited = N - this->_size;
                }
            } else {
                size_t count   = std::distance(first, last);
                size_t clamped = count <= N ? count : N;
                ::std::uninitialized_copy_n(first, clamped, begin());
                this->_size = clamped;
            }
        }

      public:
        // constructor's (done)
        constexpr stack_vector() noexcept {
            // default, do absolutely nothing
        }
        constexpr stack_vector(size_type count, const T &value) {
            // if (count)
            construct(count, value);
        }
        constexpr explicit stack_vector(size_type count) {
            // if (count)
            construct(count);
        }
        template <class It1> constexpr stack_vector(It1 first, It1 last) {
            // if (first != last)
            construct_range(first, last);
        }
        constexpr stack_vector(const stack_vector &other) {
            // if (!other.empty())
            construct_range(other.begin(), other.end());
        };
        constexpr stack_vector(stack_vector &&other) noexcept {
            // if (!other.empty())
            construct_range(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        };
        constexpr stack_vector(::std::initializer_list<value_type> init) {
            // if (init.begin() != init.end())
            construct_range(init.begin(), init.end());
        };
        template <size_t N0> constexpr stack_vector(const std::array<value_type, N0> &arr) noexcept {
            if constexpr (N0)
                construct_range(arr.begin(), arr.end());
        }

        template <size_t N0> constexpr stack_vector(const stack_vector<T, N0> &other) noexcept {
            if constexpr (N0)
                construct_range(other.begin(), other.end());
        }
        template <size_t N0> constexpr stack_vector(stack_vector<T, N0> &&other) {
            if constexpr (N0)
                construct_range(std::make_move_iterator(other.begin()), std::make_move_iterator(other.end()));
        }

        // destructor see destruct base class (done)

        // operator ='s (done)
        constexpr stack_vector &operator=(const stack_vector &other) {
            if (this == &other)
                return *this;
            size_t rhs_size   = other.size();
            size_t lhs_size   = size();
            size_t lhs_inited = lhs_size + this->_more_inited;
            if (::std::is_constant_evaluated()) {
                // we have to keep track of which union member we're dealing w/ (b/c otherwise the compiler
                // complains) :(
                if (rhs_size) {
                    // write to the union so it doesn't complain
                    this->_buf[0] = *other.begin();
                    // size_t max_count = this->_size + this->_more_inited;
                    ::std::copy(other.begin() + 1, other.begin() + rhs_size, begin() + 1);
                    size_t diff = lhs_size - rhs_size;
                    // keep track of inited values
                    if constexpr (!::std::is_trivially_destructible<value_type>::value)
                        this->_more_inited += diff;
                    this->_size = rhs_size;
                } else {
                    clear();
                }
            } else {
                if (lhs_inited >= rhs_size) {
                    iterator new_end = begin() + rhs_size;
                    ::std::copy(other.begin(), other.begin() + rhs_size, begin());
                    // destroy excess elements
                    ::stack_vector::details::destroy(new_end, begin() + lhs_inited);
                    this->_size        = rhs_size;
                    this->_more_inited = 0;
                    return *this;
                } else {
                    size_t copy_count        = rhs_size < lhs_inited ? rhs_size : lhs_inited;
                    size_t uninit_copy_count = rhs_size - copy_count;
                    // copy to initialized memory
                    //::std::copy(other.begin(), other.begin() + lhs_inited, begin());
                    ::std::copy_n(other.begin(), copy_count, begin());
                    // copy to uninitialized memory
                    ::std::uninitialized_copy_n(other.begin() + copy_count, uninit_copy_count,
                                                begin() + copy_count);
                    /*
                    ::stack_vector::details::uninitialized_copy(
                        other.begin() + lhs_inited, other.begin() + rhs_size, begin() + lhs_inited);
                        */
                    this->_size        = rhs_size;
                    this->_more_inited = 0;
                    return *this;
                }
            }
        };
        constexpr stack_vector &operator=(stack_vector &&other) noexcept {
            if (this == &other)
                return *this;
            size_t rhs_size = other.size();
            size_t lhs_size = size();
            if (::std::is_constant_evaluated()) {
                if (rhs_size) {
                    // write to the union so it doesn't complain
                    this->_buf[0] = ::std::move(*other.begin());
                    // size_t max_count = this->_size + this->_more_inited;
                    ::std::copy(::std::make_move_iterator(other.begin()) + 1,
                                std::make_move_iterator(other.begin()) + rhs_size, begin() + 1);
                    size_t diff = lhs_size - rhs_size;
                    // keep track of inited values
                    if constexpr (!::std::is_trivially_destructible<value_type>::value)
                        this->_more_inited += diff;
                    this->_size = rhs_size;
                    // clear other so it doesn't explode
                    ::std::rotate(other.begin(), other.begin() + rhs_size, other.begin() + N);
                    other._size        = N - rhs_size;
                    other._more_inited = 0;
                } else {
                    clear();
                }
            } else {
                if (lhs_size >= rhs_size) {
                    // assign
                    iterator new_end = begin() + rhs_size;
                    ::std::copy(::std::make_move_iterator(other.begin()),
                                ::std::make_move_iterator(other.begin()) + rhs_size, begin());
                    // destroy excess elements
                    ::stack_vector::details::destroy(new_end, begin() + (size() + init_size()));
                    this->_size        = rhs_size;
                    this->_more_inited = 0;
                    // clear other
                    size_t rhs_inited_size = other.size() + other.init_size();
                    for (size_t i = rhs_size; i < rhs_inited_size; i++)
                        other._buf[i].~value_type();
                    other._size        = 0;
                    other._more_inited = 0;
                } else {
                    size_t lhs_inited        = size() + init_size();
                    size_t copy_count        = rhs_size < lhs_inited ? rhs_size : lhs_inited;
                    size_t uninit_copy_count = rhs_size - copy_count;
                    // copy to initialized memory
                    //::std::copy(other.begin(), other.begin() + lhs_inited, begin());
                    ::std::copy_n(::std::make_move_iterator(other.begin()), copy_count, begin());
                    // copy to uninitialized memory
                    ::std::uninitialized_copy_n(::std::make_move_iterator(other.begin()) + copy_count,
                                                uninit_copy_count, begin() + copy_count);
                    /*
                    // copy to initialized memory
                    ::std::move(other.begin(), other.begin() + lhs_size, begin());
                    // copy to uninitialized memory
                    ::stack_vector::details::uninitialized_move(other.begin() + lhs_size,
                                                                other.begin() + rhs_size, begin() + lhs_size);
                                                                */
                    this->_size        = rhs_size;
                    this->_more_inited = 0;

                    size_t rhs_inited_size = other.size() + other.init_size();
                    for (size_t i = rhs_size; i < rhs_inited_size; i++)
                        other._buf[i].~value_type();
                    other._size        = 0;
                    other._more_inited = 0;
                }
            }
            return *this;
        };
        constexpr stack_vector &operator=(::std::initializer_list<T> ilist) {
            assign(ilist);
            return *this;
        };
        // assign's (done)
        constexpr void assign(size_type count, const T &value) {
            if (::std::is_constant_evaluated()) {
                if (count <= capacity()) {
                    size_t i         = 0;
                    size_t max_count = count <= N ? count : N;
                    // size_t inited    = (size() + init_size()) <= N ? (size() + init_size()) : N;
                    for (; i < max_count; i++)
                        this->_buf[i] = value;
                    /*
                    for (; i < max_count; i++)
                        this->_buf[i].value_type(value);
                        */
                    if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                        size_t diff = this->_size - max_count;
                        this->_more_inited += diff;
                    }
                    this->_size = max_count;
                } else {
                    if constexpr (::stack_vector::details::error_handler !=
                                  ::stack_vector::details::error_handling::_noop) {
                        return_error(false, "stack_vector cannot allocate space to assign");
                    }
                }
            } else {
                if (count <= capacity())
                    [[likely]] {
                        clear();
                        ::stack_vector::details::uninitialized_fill_n(begin(), count, value);
                        this->_size = count;
                    }
                else {
                    if constexpr (::stack_vector::details::error_handler !=
                                  ::stack_vector::details::error_handling::_noop) {
                        return_error(false, "stack_vector cannot allocate space to assign");
                    }
                }
            }
        };
        template <class It1> constexpr void assign(It1 first, It1 last) {
            if (::std::is_constant_evaluated()) {
                if constexpr (::std::is_same<::std::random_access_iterator_tag,
                                             ::std::iterator_traits<It1>::iterator_category>::value) {
                    size_t i         = 0;
                    size_t count     = last - first;
                    size_t max_count = count <= N ? count : N;
                    // size_t inited    = (size() + init_size()) <= N ? (size() + init_size()) : N;
                    for (; i < max_count; ++i)
                        this->_buf[i] = *(first + i);
                    /*
                    for (; i < inited && i < max_count; ++i)
                        this->_buf[i] = *(first + i);
                    for (; i < max_count; i++)
                        ::new (&this->_buf[i]) value_type(*(first + i));
                        */
                    // this->_buf[i].value_type(*(first + i));
                    if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                        size_t diff = this->_size - max_count;
                        this->_more_inited += diff;
                    }
                    this->_size = max_count;
                } else {
                    size_t i = 0;
                    for (; i < N && first != last; ++i, ++first)
                        this->_buf[i] = *first;
                    /*
                    size_t inited = (size() + init_size()) <= N ? (size() + init_size()) : N;
                    for (; i < inited && first != last; ++i, ++first) {
                        this->_buf[i] = *first;
                    }
                    for (; i < N && first != last; ++i, ++first) {
                        this->_buf[i].value_type(*first);
                    }
                    */
                    if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                        size_t diff = this->_size - i;
                        this->_more_inited += diff;
                    }
                    this->_size = i;
                    if constexpr (::stack_vector::details::error_handler !=
                                  ::stack_vector::details::error_handling::_noop) {
                        if (first != last) {
                            return_error(false, "stack_vector cannot allocate space to assign");
                        }
                    }
                }
            } else {
                if constexpr (::std::is_same<::std::random_access_iterator_tag,
                                             ::std::iterator_traits<It1>::iterator_category>::value) {
                    size_type insert_count = last - first;
                    if (insert_count <= capacity()) {
                        size_t copy_count = insert_count < N ? insert_count : N;
                        clear();
                        ::stack_vector::details::uninitialized_copy_n(first, copy_count, begin());
                        this->_size = insert_count;
                        if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                            this->_more_inited = 0;
                        }
                    } else {
                        if constexpr (::stack_vector::details::error_handler !=
                                      ::stack_vector::details::error_handling::_noop) {
                            return_error(false, "stack_vector cannot allocate space to insert");
                        }
                    }
                } else {
                    clear();
                    for (; first != last && size() < capacity(); ++first) {
                        unchecked_emplace_back(*first);
                    }
                    if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                        this->_more_inited = 0;
                    }
                    if constexpr (::stack_vector::details::error_handler !=
                                  ::stack_vector::details::error_handling::_noop) {
                        if (first != last) {
                            return_error(false, "stack_vector cannot allocate space to insert");
                        }
                    }
                }
            }
        };
        constexpr void assign(::std::initializer_list<T> ilist) {
            assign(ilist.begin(), ilist.end());
        };
        // append's (non-standard) (done)
        constexpr void append(size_type count, const T &value) {
            size_t space_remaining = capacity() - size();
            if (::std::is_constant_evaluated()) {
                if (count && count <= space_remaining)
                    [[likely]] {
                        size_t append_count = count < N ? count : N;
                        for (size_t i = 0; i < append_count; i++) {
                            this->_buf[size()] = value;
                            this->_size += 1;
                        }
                        if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                            this->_more_inited -= append_count;
                        }
                    }
                else {
                    if constexpr (::stack_vector::details::error_handler !=
                                  ::stack_vector::details::error_handling::_noop) {
                        return_error(false, "stack_vector cannot allocate space to insert");
                    }
                }
            } else {
                if (count)
                    [[likely]] {
                        if constexpr (::stack_vector::details::error_handler ==
                                      ::stack_vector::details::error_handling::_saturate) {
                            size_t append_count = count < N ? count : N;
                            size_t fill_count   = append_count < init_size() ? append_count : init_size();
                            size_t uninit_fill_count = append_count - fill_count;
                            ::std::fill_n(end(), fill_count, value);
                            ::stack_vector::details::uninitialized_fill_n(begin() + (size() + init_size()),
                                                                          uninit_fill_count, value);
                            this->_size += count;
                            if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                                this->_more_inited -= fill_count;
                            }
                        } else {
                            if (count <= space_remaining) {
                                //::stack_vector::details::uninitialized_fill_n(end(), count, value);
                                size_t append_count = count < N ? count : N;
                                size_t fill_count   = append_count < init_size() ? append_count : init_size();
                                size_t uninit_fill_count = append_count - fill_count;
                                ::std::fill_n(end(), fill_count, value);
                                ::stack_vector::details::uninitialized_fill_n(
                                    begin() + (size() + init_size()), uninit_fill_count, value);
                                this->_size += count;
                                if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                                    this->_more_inited -= fill_count;
                                }
                            }
                        }
                    }
                else {
                    if constexpr (::stack_vector::details::error_handler !=
                                  ::stack_vector::details::error_handling::_noop) {
                        return_error(false, "stack_vector cannot allocate space to insert");
                    }
                }
            }
        }
        template <typename It1> void append(It1 first, It1 last) {
            append_range(first, last);
        }

        // at's (done)
        [[nodiscard]] constexpr reference at(size_type pos) {
            if (pos < size())
                return this->_buf[pos];
            else
                throw ::std::out_of_range("stack_vector cannot index out of range");
        };
        [[nodiscard]] constexpr const_reference at(size_type pos) const {
            if (pos < size())
                return this->_buf[pos];
            else
                throw ::std::out_of_range("stack_vector cannot index out of range");
        };
        //[]'s (done)
        [[nodiscard]] constexpr reference operator[](size_type pos) {
            assert(pos < size());
            return this->_buf[pos];
        };
        [[nodiscard]] constexpr const_reference operator[](size_type pos) const {
            assert(pos < size());
            return this->_buf[pos];
        };
        // front
        [[nodiscard]] constexpr reference front() {
            assert(!empty());
            return this->_buf[0];
        };
        [[nodiscard]] constexpr const_reference front() const {
            assert(!empty());
            return this->_buf[0];
        };
        // back's
        [[nodiscard]] constexpr reference back() {
            assert(!empty());
            return this->_buf[size()-1];
        };
        [[nodiscard]] constexpr const_reference back() const {
            assert(!empty());
            return this->_buf[size() - 1];
        };
        // data's
        [[nodiscard]] constexpr T *data() noexcept {
            return &(this->_buf[0]);
        };
        [[nodiscard]] constexpr const T *data() const noexcept {
            return &(this->_buf[0]);
        };
        // begin's
        [[nodiscard]] constexpr iterator begin() noexcept {
            return &(this->_buf[0]);
        };

        [[nodiscard]] constexpr const_iterator begin() const noexcept {
            return &(this->_buf[0]);
        };
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept {
            return &(this->_buf[0]);
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
            return &(this->_buf[0]) + size();
        };
        [[nodiscard]] constexpr const_iterator end() const noexcept {
            return &(this->_buf[0]) + size();
        };
        [[nodiscard]] constexpr const_iterator cend() const noexcept {
            return &(this->_buf[0]) + size();
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
        // empty's
        [[nodiscard]] constexpr bool empty() const noexcept {
            return size() == 0;
        };
        // full (non standard)
        [[nodiscard]] constexpr bool full() const noexcept {
            return size() >= capacity();
        };
        // init_size (non standard)
        constexpr size_type init_size() const {
            return this->_more_inited;
        }
        // size
        constexpr size_type size() const noexcept {
            return this->_size;
        };
        // capacity (constant)
        constexpr size_type capacity() const noexcept {
            return N;
        };
        // maxthis->_size (constant)
        constexpr size_type max_size() const noexcept {
            return N;
        };
        // reserve (unimplemented or noop)
        // shrink_to_fit (unimplemented or noop)
        // ::std::uninitialized_fill(this->begin(), this->end(), Elt);
        constexpr void clear() noexcept {
            if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                if (::std::is_constant_evaluated()) {
                    // don't bother destroying things
                    this->_more_inited += this->_size;
                } else {
                    ::stack_vector::details::destroy(begin(), begin() + (size() + init_size()));
                    this->_more_inited = 0ULL;
                }
            }
            this->_size = 0ULL;
        };
        // insert's
        constexpr iterator insert(const_iterator pos, const value_type &value) {
            return emplace(pos, value);
        };
        constexpr iterator insert(const_iterator pos, value_type &&value) {
            return emplace(pos, ::std::move(value));
        };
        constexpr iterator insert(const_iterator pos, size_type count, const value_type &value) {
            const_pointer insert_ptr = &(*pos);
            const_pointer old_end    = &(*end());
            /*
            const pointer old_begin  = &(*begin());
            const pointer old_end    = &(*end());
            const pointer new_end    = old_end + count;
            const T *     value_ptr  = &value;
            */
            assert(pos >= cbegin() && pos <= cend() &&
                   "insert iterator is out of bounds of the stack_vector");

            // size_type  insert_idx         = pos - cend();
            size_type  remaining_capacity = capacity() - size();
            const bool one_at_back        = (count == 1) && (insert_ptr == old_end);
            if (count == 0) {                        // do nothing
            } else if (count > remaining_capacity) { // error?
                if constexpr (::stack_vector::details::error_handler ==
                              ::stack_vector::details::error_handling::_exception) {
                    throw std::bad_alloc("stack_vector cannot allocate to insert elements");
                } else if (::stack_vector::details::error_handler ==
                           ::stack_vector::details::error_handling::_error_code) {
                    return iterator(insert_ptr + 1);
                } else if (::stack_vector::details::error_handler ==
                           ::stack_vector::details::error_handling::_noop) {
                    return iterator(insert_ptr);
                } else if (::stack_vector::details::error_handler ==
                           ::stack_vector::details::error_handling::_saturate) {
                    if (remaining_capacity) {
                        T tmp = value;
                        if (::std::is_constant_evaluated()) {
                            size_t dist     = std::distance(cbegin(), pos);
                            size_t old_size = size();
                            ::std::copy_backward(&this->_buf[dist], &this->_buf[old_size],
                                                 &this->_buf[old_size + remaining_capacity]);
                            ::std::fill_n(&this->_buf[dist], remaining_capacity, tmp);
                            if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                                this->_more_inited -= remaining_capacity > this->_more_inited
                                                          ? this->_more_inited
                                                          : remaining_capacity;
                            }
                            this->_size += remaining_capacity;
                        } else {
                            size_t initialized = size() + init_size();
                            size_t dest        = size() + remaining_capacity; // count;
                            size_t first       = std::distance(cbegin(), pos);
                            size_t idx         = first;
                            size_t last        = first + remaining_capacity;
                            while (dest > initialized) {
                                ::new (&this->_buf[dest]) value_type(this->_buf[last]);
                                if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                                    this->_more_inited += 1;
                                }
                                dest--;
                                last--;
                            }
                            while (first != last) {
                                this->_buf[dest] = ::std::move(this->_buf[last]);
                                dest--;
                                last--;
                            }
                            if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                                this->_more_inited -= remaining_capacity > this->_more_inited
                                                          ? this->_more_inited
                                                          : remaining_capacity;
                            }
                            ::stack_vector::details::uninitialized_fill_n(&this->_buf[idx],
                                                                          remaining_capacity, tmp);
                            this->_size += remaining_capacity;
                        }
                    }
                }
            } else if (one_at_back) {
                unchecked_emplace_back(value);
            } else {
                T tmp = value;
                if (::std::is_constant_evaluated()) {
                    size_t dist     = std::distance(cbegin(), pos);
                    size_t old_size = size();
                    ::std::copy_backward(&this->_buf[dist], &this->_buf[old_size],
                                         &this->_buf[old_size + count]);
                    ::std::fill_n(&this->_buf[dist], count, tmp);
                    if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                        this->_more_inited -= count > this->_more_inited ? this->_more_inited : count;
                    }
                    this->_size += count;
                } else {
                    size_t initialized = size() + init_size();
                    size_t dest        = size() + count;
                    size_t first       = std::distance(cbegin(), pos);
                    size_t idx         = first;
                    size_t last        = first + count;
                    while (dest > initialized) {
                        ::new (&this->_buf[dest]) value_type(this->_buf[last]);
                        if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                            this->_more_inited += 1;
                        }
                        dest--;
                        last--;
                    }
                    while (first != last) {
                        this->_buf[dest] = ::std::move(this->_buf[last]);
                        dest--;
                        last--;
                    }
                    if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                        this->_more_inited -= count > this->_more_inited ? this->_more_inited : count;
                    }
                    ::stack_vector::details::uninitialized_fill_n(&this->_buf[idx], count, tmp);
                    this->_size += count;
                }
            }
            return iterator(insert_ptr);
        };
        template <class InputIt> constexpr iterator insert(const_iterator pos, InputIt first, InputIt last) {
            return insert_range(pos, first, last);
        };
        constexpr iterator insert(const_iterator pos, ::std::initializer_list<T> ilist) {
            return insert_range(pos, ilist.begin(), ilist.end());
        };
        // emplace's
        template <class... Args> constexpr iterator emplace(const_iterator pos, Args &&... args) {
            if (::std::is_constant_evaluated()) {
                size_type insert_idx = pos - cbegin();
                iterator  ret_it     = begin() + insert_idx;
                if (!full()) {
                    ::std::copy_backward(begin() + insert_idx, end(), end() + 1);
                    *ret_it = value_type(::std::forward<Args>(args)...);
                    if constexpr (::std::is_trivially_destructible<value_type>::value)
                        this->_more_inited -= 1;
                    this->_size += 1;
                    return ret_it;
                } else {
                    return ret_it = return_error(ret_it, "stack_vector cannot allocate to emplace elements");
                }
            } else {
                size_type insert_idx = pos - cbegin();
                iterator  ret_it     = begin() + insert_idx;
                if (pos == cend()) { // special case for empty vector
                    if (!full())
                        [[likely]] {
                            ::new ((void *)ret_it) T(::std::forward<Args>(args)...);
                            this->_size += 1;
                            return ret_it;
                        }
                    else { // error?
                        return ret_it =
                                   return_error(ret_it, "stack_vector cannot allocate to emplace elements");
                    }
                }

                assert(pos >= cbegin() && "insertion iterator is out of bounds.");
                assert(pos <= cend() && "inserting past the end of the stack_vector.");
                // if full we back out
                if (full()) {
                    return ret_it = return_error(ret_it, "stack_vector cannot allocate to insert elements");
                }
                // T tmp = T(::std::forward<Args>(args)...);
                // placement new back item, eg... ...insert here, a, b, c, end -> ...insert
                // here, a, a, b, c, (end)
                if constexpr (!::std::is_trivially_destructible<value_type>::value) {
                    if (init_size()) {
                        *end() = value_type(::std::move(back()));
                    } else {
                        ::new ((void *)end()) value_type(::std::move(back()));
                    }
                    this->_more_inited -= this->_more_inited ? 1 : 0;
                } else {
                    ::new ((void *)end()) value_type(::std::move(back()));
                }
                // first last, destination
                ::std::move_backward(begin() + insert_idx, end() - 1, end());
                // up the size
                this->_size += 1;
                // insert the value
                ::new ((void *)ret_it) value_type(::std::forward<Args>(args)...);
                //*ret_it = ::std::move(tmp);
                return ret_it;
            }
        };

        // push_back's
        constexpr void push_back(const value_type &value) {
            emplace_back(::std::forward<const value_type &>(value));
        }
        constexpr void push_back(value_type &&value) {
            emplace_back(::std::forward<value_type &&>(value));
        };
        // emplace_back's
        template <class... Args> constexpr reference emplace_back(Args &&... args) {
            if (::std::is_constant_evaluated()) {
                size_t idx = size();
                if (size() < capacity()) {
                    this->_buf[idx] = value_type(::std::forward<Args>(args)...);
                    this->_more_inited -= (this->_more_inited > 0);
                    this->_size += 1;
                    return this->_buf[idx];
                } else {
                    if constexpr (::stack_vector::details::error_handler ==
                                  ::stack_vector::details::error_handling::_exception) {
                        throw std::bad_alloc("stack_vector cannot allocate to insert elements");
                    }
                    return this->_buf[idx];
                }
            } else {
                iterator it = end();
                if (size() < capacity())
                    [[likely]] {
                        if constexpr (::std::is_trivially_destructible<value_type>::value) {
                            ::new ((void *)it) value_type(::std::forward<Args>(args)...);
                        } else {
                            if (init_size()) {
                                *it = value_type(::std::forward<Args>(args)...);
                            } else {
                                ::new ((void *)it) value_type(::std::forward<Args>(args)...);
                            }
                            this->_more_inited -= (this->_more_inited > 0);
                        }
                        this->_size += 1;
                    }
                else { // error?
                    if constexpr (::stack_vector::details::error_handler ==
                                  ::stack_vector::details::error_handling::_exception) {
                        throw std::bad_alloc("stack_vector cannot allocate to insert elements");
                    }
                }
                return *it;
            }
        };

        template <class... Args> constexpr reference unchecked_emplace_back(Args &&... args) {
            if (::std::is_constant_evaluated()) {
                size_t idx      = size();
                this->_buf[idx] = value_type(::std::forward<Args>(args)...);
                this->_more_inited -= (this->_more_inited > 0);
                this->_size += 1;
                return this->_buf[idx];
            } else {
                iterator it = end();
                if constexpr (::std::is_trivially_destructible<value_type>::value) {
                    ::new ((void *)it) value_type(::std::forward<Args>(args)...);
                } else {
                    if (init_size()) {
                        *it = value_type(::std::forward<Args>(args)...);
                    } else {
                        ::new ((void *)it) value_type(::std::forward<Args>(args)...);
                    }
                    this->_more_inited -= (this->_more_inited > 0);
                }
                this->_size += 1;
                return *it;
            }
        };

        // shove_back's (unchecked_push_back)
        constexpr void shove_back(const value_type &value) {
            unchecked_emplace_back(::std::forward<const value_type &>(value));
        }

        constexpr void shove_back(value_type &&value) {
            unchecked_emplace_back(::std::forward<value_type &&>(value));
        }

        // pop_back's
        constexpr void pop_back() {
            if (::std::is_constant_evaluated()) {
                if (size()) {
                    if constexpr (::std::is_trivially_destructible<value_type>::value) {
                        this->_size -= 1;
                    } else {
                        this->_size -= 1;
                        this->_more_inited += 1;
                    }
                } else {
                    if constexpr (::stack_vector::details::error_handler ==
                                  ::stack_vector::details::error_handling::_exception) {
                        throw std::domain_error("stack_vector cannot pop_back when empty");
                    }
                }
            } else {
                if (size())
                    [[likely]] {
                        if constexpr (::std::is_trivially_destructible<value_type>::value) {
                            this->_size -= 1;
                        } else {
                            // end()->~value_type(); // destroy the tailing value
                            this->_size -= 1;
                            (end() + init_size())->~value_type();
                            // this->_more_inited += 1;
                        }
                    }
                else { // error?
                    if constexpr (::stack_vector::details::error_handler ==
                                  ::stack_vector::details::error_handling::_exception) {
                        throw std::domain_error("stack_vector cannot pop_back when empty");
                    }
                }
            }
        };
        // erase's
        constexpr iterator
        erase(const_iterator pos) noexcept(::std::is_nothrow_move_assignable_v<value_type>) {
            size_type erase_idx = pos - cbegin();
            assert(pos >= cbegin() && pos <= cend() && "erase iterator is out of bounds of the stack_vector");
            if (::std::is_constant_evaluated()) {
                if (erase_idx < size()) {
                    this->_size -= 1;
                    size_t nsize = this->_size;
                    for (size_t i = erase_idx; i < nsize; i++) {
                        this->_buf[i] = this->_buf[i + 1];
                    }
                }
                /*
                ::std::rotate(pos, pos + 1, cend());
                */
                if constexpr (::std::is_trivially_destructible<value_type>::value)
                    this->_more_inited += 1;

                return begin() + erase_idx;
            } else {
                // move on top
                iterator first = begin() + erase_idx + 1;
                iterator last  = end();
                iterator dest  = begin() + erase_idx;
                for (; first != last; ++dest, (void)++first) {
                    *dest = ::std::move(*first);
                }
                //::stack_vector::details::destroy_at(end() - 1);
                ::stack_vector::details::destroy(end() - 1, begin() + (size() + init_size()));

                if constexpr (::std::is_trivially_destructible<value_type>::value)
                    this->_more_inited = 0;
                this->_size -= 1;
                return begin() + erase_idx;
            }
        }

        constexpr iterator
        erase(const_iterator first,
              const_iterator last) noexcept(::std::is_nothrow_move_assignable_v<value_type>) {
            assert(first >= cbegin() && first <= cend() &&
                   "first erase iterator is out of bounds of the stack_vector");
            assert(last >= cbegin() && last <= cend() &&
                   "last erase iterator is out of bounds of the stack_vector");
            if (::std::is_constant_evaluated()) {
                size_type erase_idx = first - cbegin();
                if (first != last) {
                    size_t offset = std::distance(first, last);
                    this->_size -= offset;
                    size_t nsize = this->_size;
                    for (size_t i = erase_idx; i < nsize; i++) {
                        this->_buf[i] = this->_buf[i + offset];
                    }

                    if constexpr (::std::is_trivially_destructible<value_type>::value)
                        this->_more_inited += offset;
                }
                return begin() + erase_idx;
            } else {
                size_type erase_idx = first - cbegin();
                if (first != last) {
                    size_type erase_count = last - first;
                    iterator  _first      = begin() + (erase_idx + erase_count);
                    iterator  _last       = end();
                    iterator  dest        = begin() + erase_idx;
                    for (; _first != _last; ++dest, (void)++_first) {
                        *dest = ::std::move(*_first);
                    }
                    ::stack_vector::details::destroy(end() - erase_count, begin() + (size() + init_size()));
                    this->_size -= erase_count;
                    if constexpr (::std::is_trivially_destructible<value_type>::value)
                        this->_more_inited = 0;
                }
                return begin() + erase_idx;
            }
        }

        // resize's (unimplemented or noop)
        // swap's
        constexpr void swap(stack_vector &other) noexcept {
            if (this == &other)
                return;
            if constexpr (::std::is_trivially_copyable<value_type>::value) {
                ::std::swap(this->_buf, other._buf);
                // swap the sizes
                ::std::swap(this->_size, other._size);
                ::std::swap(this->_more_inited, other._more_inited);
            } else {
                stack_vector tmp = *this; // meh
                assign(other);
                other.assign(tmp);
            }
        }
    };

    template <class T, size_t N0, size_t N1>
    [[nodiscard]] constexpr ::stack_vector::stack_vector<T, N0 + N1> __forceinline append(
        const ::stack_vector::stack_vector<T, N0> &left, const ::stack_vector::stack_vector<T, N1> &right) {
        ::stack_vector::stack_vector<T, N0 + N1> ret;
        ret.append(left.begin(), left.end());
        ret.append(right.begin(), right.end());
        return ret;
    }
} // namespace stack_vector

// non-members
template <class T, size_t N0, size_t N1>
[[nodiscard]] constexpr bool operator==(const stack_vector::stack_vector<T, N0> &left,
                                        const stack_vector::stack_vector<T, N1> &right) {
    return left.size() == right.size() && ::std::equal(left.begin(), left.end(), right.begin());
}

template <class T, size_t N0, size_t N1>
[[nodiscard]] constexpr bool operator!=(const stack_vector::stack_vector<T, N0> &left,
                                        const stack_vector::stack_vector<T, N1> &right) {
    return !(left == right);
}

template <class T, size_t N0, size_t N1>
[[nodiscard]] constexpr bool operator<(const stack_vector::stack_vector<T, N0> &left,
                                       const stack_vector::stack_vector<T, N1> &right) {
    return ::std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end());
}

template <class T, size_t N0, size_t N1>
[[nodiscard]] constexpr bool operator>(const stack_vector::stack_vector<T, N0> &left,
                                       const stack_vector::stack_vector<T, N1> &right) {
    return right < left;
}

template <class T, size_t N0, size_t N1>
[[nodiscard]] constexpr bool operator<=(const stack_vector::stack_vector<T, N0> &left,
                                        const stack_vector::stack_vector<T, N1> &right) {
    return !(right < left);
}

template <class T, size_t N0, size_t N1>
[[nodiscard]] constexpr bool operator>=(const stack_vector::stack_vector<T, N0> &left,
                                        const stack_vector::stack_vector<T, N1> &right) {
    return !(left < right);
}

namespace std {
    // conditional erases
    template <class T, size_t N, class U>
    constexpr typename stack_vector::stack_vector<T, N>::size_type erase(stack_vector::stack_vector<T, N> &c,
                                                                         const U &value) {
        auto it = ::std::remove(c.begin(), c.end(), value);
        auto r  = ::std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    }

    template <class T, size_t N, class Pred>
    constexpr typename stack_vector::stack_vector<T, N>::size_type
    erase_if(stack_vector::stack_vector<T, N> &c, Pred pred) {
        auto it = ::std::remove_if(c.begin(), c.end(), pred);
        auto r  = ::std::distance(it, c.end());
        c.erase(it, c.end());
        return r;
    };

    template <class T, size_t N>
    constexpr void swap(::stack_vector::stack_vector<T, N> &left,
                        ::stack_vector::stack_vector<T, N> &right) noexcept {
        left.swap(right);
    }
}; // namespace std