//
// Created by arnoldm on 24.10.24.
//

#ifndef DYN_ARRAY_H
#define DYN_ARRAY_H

#include<cassert>
#include<stdexcept>

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
            other.clear();
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
            other.clear();
            return *this;
        }

        dyn_array & operator=(std::initializer_list<value_type> init_list) {
            if (init_list.size() > capacity()) {
                throw std::length_error("Initializer list size is greater than dyn_array capacity");
            }
            clear();
            for(auto && value : init_list)
                emplace_back_unchecked(std::move(value));
            return *this;
        }

        friend bool operator==(const dyn_array &lhs, const dyn_array &rhs) {
            return lhs.size_ == rhs.size_ && std::ranges::equal(lhs, rhs);
        }

        friend bool operator!=(const dyn_array &lhs, const dyn_array &rhs) {
            return !(lhs == rhs);
        }

        //front
        [[nodiscard]] reference front() noexcept {
            assert((size_ > 0) && "front undefined if empty");
            return data()[0];
        }

        [[nodiscard]] constexpr const_reference front() const noexcept {
            assert((size_ > 0) && "front undefined if empty");
            return data()[0];
        }

        //back
        [[nodiscard]] reference back() noexcept {
            assert((size_ > 0) && "back undefined if empty");
            return data()[size_ - 1];
        }

        [[nodiscard]] constexpr const_reference back() const noexcept {
            assert((size_ > 0) && "back undefined if empty");
            return data()[size_ - 1];
        }

        //begin
        [[nodiscard]] iterator begin() noexcept { return iterator(data()); }
        [[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator(data()); }
        [[nodiscard]] constexpr const_iterator cbegin() const noexcept { return const_iterator(data()); }

        //end
        [[nodiscard]] iterator end() noexcept { return iterator(data() + size()); }
        [[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator(data() + size()); }
        [[nodiscard]] constexpr const_iterator cend() const noexcept { return const_iterator(data() + size()); }

        //empty
        [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }
        [[nodiscard]] constexpr bool full() const noexcept { return size() == capacity(); }

        //size
        [[nodiscard]] size_type size() const noexcept { return size_; }

        //max_size
        [[nodiscard]] size_type max_size() const noexcept { return capacity(); }

        //capacity
        [[nodiscard]] size_type capacity() const noexcept { return size_type(Capacity); }

        [[nodiscard]] constexpr pointer data() noexcept { return reinterpret_cast<pointer>(data_); }
        [[nodiscard]] constexpr const_pointer data() const noexcept { return reinterpret_cast<const_pointer>(data_); }

        [[nodiscard]] reference at(std::size_t index) {
            if (index >= size())
                throw std::out_of_range("index out of range");
            return data()[index];
        }
        [[nodiscard]] constexpr const_reference at(std::size_t index) const {
            if (index >= size())
                throw std::out_of_range("index out of range");
            return data()[index];
        }
        [[nodiscard]] reference operator[](std::size_t index) noexcept {
            assert((index < size()) && "operator[] index out of range");
            return data()[index];
        }
        [[nodiscard]] constexpr const_reference operator[](std::size_t index) const noexcept {
            assert((index < size()) && "operator[] index out of range");
            return data()[index];
        }

        void swap(dyn_array &other) noexcept {
            dyn_array* shorter = this;
            dyn_array* longer = &other;
            if (longer->size() < shorter->size())
                std::swap(longer, shorter);
            std::swap_ranges(shorter->begin(), shorter->end(), longer->begin());
            std::move(longer->begin() + shorter->size(), longer->end(), std::back_inserter(*shorter));
            auto longer_size = longer->size();
            longer->resize(shorter->size());
            shorter->size_ = longer_size;
        }

        void fill(const value_type &value) {
            std::fill(begin(), end(), value);
        }

        void pop_back() noexcept {
            assert((!empty()) && "pop_back undefined if empty");
            pop_back_unchecked();
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
            assert((!full()) && "capacity exceeded");
            if (full())
                throw std::out_of_range("capacity exceeded");
            push_back_unchecked(value);
        }

        void push_back(value_type&& value) {
            assert((!full()) && "capacity exceeded");
            if (full())
                throw std::out_of_range("capacity exceeded");
            emplace_back_unchecked(std::move(value));
        }

        void emplace_back(auto && ... args) {
            assert((!full()) && "capacity exceeded");
            if (full())
                throw std::out_of_range("capacity exceeded");
            emplace_back_unchecked(std::forward<decltype(args)>(args)...);
        }

        iterator insert(iterator pos, const value_type& value) {
            assert((!full()) && "capacity exceeded");
            assert((begin() <= pos && pos <= end()) && "insert: pos iterator of invalid range");
            if (full())
                throw std::out_of_range("capacity exceeded");
            if (pos == end())
                push_back(value);
            else {
                std::construct_at(&data()[size_]);
                std::move_backward(pos, begin() + size_, end() + 1);
                ++size_;
                *pos = value;
            }
            return pos;
        }

        iterator insert_space(iterator pos, size_type count) {
            assert((size() + count < capacity()) && "insert_space: capacity exceeded");
            assert((begin() <= pos && pos <= end()) && "insert: pos iterator of invalid range");
            if (count == 0)
                return pos;
            if (size() + count > capacity())
                throw std::out_of_range("capacity exceeded");
            auto old_size = size();
            resize(size() + count);
            std::move_backward(pos, begin() + old_size, end());
            return pos;
        }

        iterator erase(iterator pos) {
            assert((begin() <= pos && pos <= end()) && "erase: pos iterator of invalid range");
            if (empty())
                return end();
            if (pos + 1 != end())
                std::move(pos + 1, end(), pos);
            --size_;
            std::destroy_at(&data()[size_]);
            return pos;
        }

        const_iterator erase(const_iterator first, const_iterator last) {
            assert((cbegin() <= first && first <= cend()) && "erase: first iterator of invalid range");
            assert((cbegin() <= last && last <= cend()) && "erase: last iterator of invalid range");
            assert((first <= last) && "erase: first iterator > last iterator");
            std::move(last, cend(), iterator(first));
            resize(size() - std::distance(first, last));
            return first;
        }

        void resize(std::size_t new_size, value_type const & init = value_type()) {
            assert((new_size <= capacity()) && "resize capacity exceeded");
            while(size() > new_size)
                pop_back_unchecked();
            while(size() < new_size)
                push_back_unchecked(init);
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
        void pop_back_unchecked() noexcept {
            --size_;
            std::destroy_at(&data()[size_]);
        }

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

