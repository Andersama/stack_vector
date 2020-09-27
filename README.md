# stack_vector
A fixed capacity vector, for dynamic arrays of maximum known size

Implements most of vector functions except those dealing with resizing.

TODO:
* Allow some form of error for when inserts / push_backs go beyond the capacity

Currently eaither noop's or saturates capacity when insertions are too large.

## Extras
Given a known size at compile time it may be handy to have a few extra tools available for inserting and appending data.
```c
        // shove_back's (unchecked_push_back)
        constexpr void shove_back(const T &value);
        constexpr void shove_back(T &&value);
        template <class... Args> constexpr reference unchecked_emplace_back(Args &&... args);
```
```c
        void append(size_type count, const T &value);
        template <typename some_iterator> void append(some_iterator first, some_iterator last);
```
