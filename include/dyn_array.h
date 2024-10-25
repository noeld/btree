//
// Created by arnoldm on 24.10.24.
//

#ifndef DYN_ARRAY_H
#define DYN_ARRAY_H

#include<cassert>

namespace bt {
    /**
    * Like a std::array but knows its size.
    */
    template<typename Value, std::size_t Capacity, typename Size = std::size_t>
    requires std::is_default_constructible_v<Value>
    class dyn_array {
    public:

        typedef Value value_type;
        typedef value_type *pointer;
        typedef const value_type *const_pointer;
        typedef value_type &reference;
        typedef const value_type &const_reference;
        typedef value_type *iterator;
        typedef const value_type *const_iterator;
        typedef Size size_type;
        typedef std::ptrdiff_t difference_type;
        typedef std::reverse_iterator<iterator> reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        dyn_array() = default;

        dyn_array(const dyn_array &other)
        {
            for(auto const & e : other)
                push_back_unchecked(e);
        }

        dyn_array(dyn_array &&other) {
            for(auto&& e : other)
                emplace_back_unchecked(std::move(e));
        }

        dyn_array(std::initializer_list<value_type> init_list) {
            if (init_list.size() > capacity()) {
                throw std::length_error("Initializer list size is greater than dyn_array capacity");
            }
            for(auto && value : init_list)
                emplace_back_unchecked(std::move(value));
        }

        ~dyn_array() noexcept {
            clear();
        }

        dyn_array & operator=(const dyn_array &other) {
            if (this == &other)
                return *this;
            clear();
            for(auto const & value : other)
                push_back_unchecked(value);
            return *this;
        }

        dyn_array & operator=(dyn_array &&other) noexcept {
            if (this == &other)
                return *this;
            clear();
            for(auto && value : other)
                emplace_back_unchecked(std::move(value));
            return *this;
        }

        //front
        [[nodiscard]] reference front() noexcept {
            assert(("front undefined if empty", size_ > 0));
            return data()[0];
        }

        [[nodiscard]] constexpr const_reference front() const noexcept {
            assert(("front undefined if empty", size_ > 0));
            return data()[0];
        }

        //back
        [[nodiscard]] reference back() noexcept {
            assert(("back undefined if empty", size_ > 0));
            return data()[size_ - 1];
        }

        [[nodiscard]] constexpr const_reference back() const noexcept {
            assert(("back undefined if empty", size_ > 0));
            return data()[size_ - 1];
        }

        //begin
        [[nodiscard]] iterator begin() noexcept { return iterator(data()); }
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator(data()); }

        //end
        [[nodiscard]] iterator end() noexcept { return iterator(data() + size_); }
        [[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator(data() + size_); }

        //empty
        [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }

        //size
        [[nodiscard]] std::size_t size() const noexcept { return static_cast<std::size_t>(size_); }

        //max_size
        [[nodiscard]] std::size_t max_size() const noexcept { return Capacity; }

        //capacity
        [[nodiscard]] std::size_t capacity() const noexcept { return Capacity; }

        [[nodiscard]] constexpr pointer data() noexcept { return reinterpret_cast<pointer>(data_); }
        [[nodiscard]] constexpr const_pointer data() const noexcept { return reinterpret_cast<const_pointer>(data_); }

        [[nodiscard]] reference at(size_type index) {
            if (index >= size_)
                throw std::out_of_range("index out of range");
            return data()[index];
        }
        [[nodiscard]] constexpr const_reference at(size_type index) const {
            if (index >= size_)
                throw std::out_of_range("index out of range");
            return data()[index];
        }
        [[nodiscard]] reference operator[](size_type index) noexcept {
            assert(("operator[] index out of range", index < size_));
            return data()[index];
        }
        [[nodiscard]] constexpr const_reference operator[](size_type index) const noexcept {
            assert(("operator[] index out of range", index < size_));
            return data()[index];
        }

        void swap(dyn_array &other) noexcept {
            dyn_array* shorter = this;
            dyn_array* longer = &other;
            if (longer->size() < shorter->size())
                std::swap(longer, shorter);
            std::swap_ranges(shorter->begin(), shorter->end(), longer->begin());
            std::copy(longer->begin() + shorter->size(), longer->end(), std::back_inserter(*shorter));
            std::swap(size_, other.size_);
        }

        void fill(const value_type &value) {
            std::fill(begin(), end(), value);
        }

        void pop_back() noexcept {
            assert(("pop_back undefined if empty", size_ > 0));
            --size_;
            std::destroy_at(&data()[size_]);
        }

        void clear() noexcept {
            if constexpr (!std::is_trivially_destructible_v<value_type>) {
                while(!empty())
                    pop_back();
            } else {
                size_ = 0;
            }
        }

        void push_back(const value_type& value) {
            assert( ("capacity exceeded", size_ < capacity()) );
            if (size_ >= capacity())
                throw std::out_of_range("capacity exceeded");
            push_back_unchecked(value);
        }

        void push_back(value_type&& value) {
            assert( ("capacity exceeded", size_ < capacity()) );
            if (size_ >= capacity())
                throw std::out_of_range("capacity exceeded");
            emplace_back_unchecked(std::move(value));
        }

        void emplace_back(auto && ... args) {
            assert( ("capacity exceeded", size_ < capacity()) );
            if (size_ >= capacity())
                throw std::out_of_range("capacity exceeded");
            emplace_back_unchecked(std::forward<decltype(args)>(args)...);
        }

        iterator insert(iterator pos, const value_type& value) {
            assert( ("capacity exceeded", size_ < capacity()) );
            if (size_ >= capacity())
                throw std::out_of_range("capacity exceeded");
            if (pos == end())
                push_back(value);
            else {
                std::construct_at(&data()[size_]);
                std::move_backward(pos + 1, begin() + size_, begin() + size_ + 1);
                ++size_;
                *pos = value;
            }
            return pos;
        }

        iterator erase(iterator pos) {
            if (pos + 1 != end())
                std::move(pos + 1, end(), pos);
            --size_;
            std::destroy_at(&data()[size_]);
            return pos;
        }

    protected:
        void push_back_unchecked(const value_type& value) {
            std::construct_at(&(data()[size_]), value);
            ++size_;
        }
        void emplace_back_unchecked(auto && ... args) {
            std::construct_at(&(data()[size_]), std::forward<decltype(args)>(args)...);
            ++size_;
        }
        // void resize(std::size_t new_size, value_type&& value = value_type()) {
        //     assert( ("capacity exceeded", new_size < capacity()) );
        // }

    private:
        size_type size_ { 0 };
        alignas(value_type) std::byte data_[Capacity * sizeof(value_type)] = {static_cast<std::byte>(0)};
        // std::aligned_storage<sizeof(value_type), alignof(value_type)> data_[Capacity]; // deprecated in C++23
    };
} // bt

namespace std {
    template<typename Value, std::size_t Capacity, typename Size>
    void swap(bt::dyn_array<Value, Capacity, Size> &lhs, bt::dyn_array<Value, Capacity, Size> &rhs) noexcept {
        lhs.swap(rhs);
    }

    template<typename Value, std::size_t Capacity, typename Size>
    typename bt::dyn_array<Value, Capacity, Size>::iterator begin(bt::dyn_array<Value, Capacity, Size>& a) noexcept {
        return a.begin();
    }
    template<typename Value, std::size_t Capacity, typename Size>
    typename bt::dyn_array<Value, Capacity, Size>::const_iterator begin(bt::dyn_array<Value, Capacity, Size> const & a) noexcept {
        return a.begin();
    }
    template<typename Value, std::size_t Capacity, typename Size>
    typename bt::dyn_array<Value, Capacity, Size>::iterator end(bt::dyn_array<Value, Capacity, Size>& a) noexcept {
        return a.end();
    }
    template<typename Value, std::size_t Capacity, typename Size>
    typename bt::dyn_array<Value, Capacity, Size>::const_iterator end(bt::dyn_array<Value, Capacity, Size> const & a) noexcept {
        return a.end();
    }
    template<typename Value, std::size_t Capacity, typename Size>
    std::size_t size(bt::dyn_array<Value, Capacity, Size> const & a) noexcept {
        return a.size();
    }
}

#endif //DYN_ARRAY_H
