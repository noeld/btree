//
// Created by arnoldm on 23.10.24.
//

#ifndef BTREE_H
#define BTREE_H

#include <functional>
#include <variant>
#include <iosfwd>
#include <string>
#include <algorithm>
#include <numeric>
#include <sstream>
#include "dyn_array.h"

namespace bt {
    class btree_test_class;

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    class btree;

    template<typename Btree_traits, bool Is_leaf>
    class btree_node;

    template<typename Btree_traits>
    class btree_iterator;

    template<typename Btree_traits>
    class btree_internal_node;

    template<typename Btree_traits>
    class btree_leaf_node;

    template<typename Btree_traits, bool Is_leaf>
    class btree_node {
    public:
        using this_type = btree_node;
        using key_type = typename Btree_traits::key_type;
        using value_type = typename Btree_traits::value_type;
        using index_type = typename Btree_traits::index_type;
        using btree_type = btree<typename Btree_traits::key_type, typename Btree_traits::value_type, typename Btree_traits::index_type,
        Btree_traits::internal_order, Btree_traits::leaf_order>;
        using key_store_type = bt::dyn_array<key_type, Btree_traits::template get_order<Is_leaf>(), index_type>;

        static constexpr index_type INVALID_INDEX = std::numeric_limits<index_type>::max();
        static constexpr bool is_leaf() { return Is_leaf; }

        static constexpr size_t order() noexcept { return Btree_traits::template get_order<Is_leaf>(); }

        btree_node() = default;

        explicit btree_node(index_type index,
                   const index_type parent_index = INVALID_INDEX,
                   key_store_type&& keys = key_store_type())
            : index_(index),
              parent_index_(parent_index), keys_(std::move(keys)) {
        }

        btree_node(const btree_node &other) = default;

        btree_node & operator=(const btree_node &other) = default;

        [[nodiscard]] const index_type &index() const noexcept { return index_; }
        [[nodiscard]] bool has_parent() const noexcept { return parent_index() != INVALID_INDEX; }
        [[nodiscard]] const index_type &parent_index() const noexcept { return parent_index_; }

        [[nodiscard]] index_type size() const { return keys_.size(); }

        [[nodiscard]] key_store_type& keys() { return keys_; }
        [[nodiscard]] const key_store_type& keys() const { return keys_; }

        auto mark_deleted() {
            keys().clear();
            set_parent_index(INVALID_INDEX);
        }

    protected:
        void set_index(const index_type &index) {
            index_ = index;
        }
        void set_parent_index(const index_type &index) {
            parent_index_ = index;
        }

    private:
        friend btree_type;
        friend btree_test_class;
        index_type index_ = INVALID_INDEX;
        index_type parent_index_ = INVALID_INDEX;
        key_store_type keys_;
    };

    template<typename Btree_traits>
    class btree_internal_node : public btree_node<Btree_traits, false> {
    public:
        using key_type = typename Btree_traits::key_type;
        using value_type = typename Btree_traits::value_type;
        using index_type = typename Btree_traits::index_type;
        using this_type = btree_internal_node;
        using base_type = btree_node<Btree_traits, false>;
        using btree_type = btree<key_type, value_type, index_type, Btree_traits::internal_order, Btree_traits::leaf_order>;
        using index_store_type = bt::dyn_array<index_type, Btree_traits::internal_order + 1, index_type>;

        using base_type::INVALID_INDEX;

        btree_internal_node() = default;

        explicit btree_internal_node(index_type index,
                   const index_type parent_index = INVALID_INDEX,
                   typename base_type::key_store_type&& keys = {},
                   index_store_type&& indices = {})
                       : base_type(index, parent_index, std::move(keys)), child_indices_(std::move(indices)) {
        }

        btree_internal_node(const btree_internal_node &other) = default;

        btree_internal_node & operator=(const btree_internal_node &other) = default;

        [[nodiscard]] index_store_type& child_indices() { return child_indices_; }
        [[nodiscard]] const index_store_type& child_indices() const { return child_indices_; }

        [[nodiscard]] auto iterators_for_index(index_type index)
            -> std::pair<typename base_type::key_store_type::iterator, typename index_store_type::iterator> {
            auto index_it = std::ranges::find(child_indices(), index);
            assert((index_it != child_indices().end() && *index_it == index) && "index is no child of this node");
            auto key_it = index_it == child_indices().begin()
                              ? this->keys().end()
                              : this->keys().begin() + std::distance(child_indices().begin(), index_it) - 1;
            return std::make_pair(key_it, index_it);
        }

        [[nodiscard]] auto iterators_for_index(index_type index) const
            -> std::pair<typename base_type::key_store_type::const_iterator, typename index_store_type::const_iterator> {
            auto index_it = std::ranges::find(child_indices(), index);
            assert((index_it != child_indices().end() && *index_it == index) && "index is no child of this node");
            auto key_it = index_it == child_indices().begin()
                              ? this->keys().end()
                              : this->keys().begin() + std::distance(child_indices().begin(), index_it) - 1;
            return std::make_pair(key_it, index_it);
        }

        [[nodiscard]] auto siblings_for_index(index_type index) const -> std::pair<index_type, index_type> {
            auto [key_it, index_it] = this->iterators_for_index(index);
            auto prev_index = index_it != child_indices().begin() ? *(index_it - 1): INVALID_INDEX;
            auto next_index = index_it + 1 != child_indices().end() ? *(index_it + 1) : INVALID_INDEX;
            return {prev_index, next_index};
        }

        auto mark_deleted() {
            base_type::mark_deleted();
            child_indices().clear();
        }

        // auto accept(btree_type& tree, auto& visitor) -> void {
        //     visitor(*this);
        //     for (auto index: child_indices_) {
        //         std::visit([&visitor, &tree](auto& node) {
        //             node.accept(tree, visitor);
        //         }, tree.node(index));
        //     }
        // }

    protected:

    private:
        friend btree_type;
        friend btree_test_class;
        index_store_type child_indices_;
    };

    template<typename Btree_traits>
    class btree_leaf_node : public btree_node<Btree_traits, true> {
    public:
        using key_type = typename Btree_traits::key_type;
        using value_type = typename Btree_traits::value_type;
        using index_type = typename Btree_traits::index_type;
        using this_type = btree_leaf_node;
        using base_type = btree_node<Btree_traits, true>;
        using btree_type = btree<key_type, value_type, index_type, Btree_traits::internal_order, Btree_traits::leaf_order>;
        using value_store_type = bt::dyn_array<value_type, Btree_traits::leaf_order, index_type>;

        using base_type::INVALID_INDEX;

        btree_leaf_node() = default;

        btree_leaf_node(const btree_leaf_node &other) = default;

        btree_leaf_node(index_type index,
                        index_type parent_index,
                        index_type previous_leaf_index = base_type::INVALID_INDEX,
                        index_type next_leaf_index = base_type::INVALID_INDEX,
                        typename base_type::key_store_type &&keys = {},
                        value_store_type &&values = {})
            : btree_node<Btree_traits, true>(index, parent_index, std::move(keys)),
              previous_leaf_index_(previous_leaf_index),
              next_leaf_index_(next_leaf_index),
              values_(std::move(values)) {
        }

        btree_leaf_node & operator=(const btree_leaf_node &other) = default;

        // explicit btree_leaf_node(index_type index = INVALID_INDEX, index_type parent_index = INVALID_INDEX)
        //     : base_type(index, parent_index) {
        // }

        [[nodiscard]] static constexpr bool is_leaf() { return true; }

        [[nodiscard]] value_store_type& values() { return values_; }
        [[nodiscard]] const value_store_type& values() const { return values_; }

        // auto accept(btree_type &tree, auto &visitor) -> void {
        //     visitor(*this);
        // }

        decltype(auto) operator[](index_type index) {
            return std::tie(this->keys()[index], values()[index]);
        }

        [[nodiscard]]index_type previous_leaf_index() const { return previous_leaf_index_; }

        [[nodiscard]] auto has_previous_leaf_index() const { return previous_leaf_index() != INVALID_INDEX; }

        void set_previous_leaf_index(const index_type &previous_leaf_index) {
            previous_leaf_index_ = previous_leaf_index;
        }

        [[nodiscard]] index_type next_leaf_index() const { return next_leaf_index_; }

        [[nodiscard]] auto has_next_leaf_index() const { return next_leaf_index() != INVALID_INDEX; }

        void set_next_leaf_index(const index_type &next_leaf_index) {
            next_leaf_index_ = next_leaf_index;
        }

        auto mark_deleted() {
            base_type::mark_deleted();
            values().clear();
            set_previous_leaf_index(INVALID_INDEX);
            set_next_leaf_index(INVALID_INDEX);
        }

    private:
        friend btree_type;
        friend btree_test_class;
        index_type previous_leaf_index_{base_type::INVALID_INDEX};
        index_type next_leaf_index_{base_type::INVALID_INDEX};
        value_store_type values_;
    };

    template<typename Key, typename Value, typename Index, std::size_t Internal_order, std::size_t Leaf_order>
    struct traits_type {
        using key_type = Key;
        using value_type = Value;
        using index_type = Index;
        static constexpr std::size_t internal_order = Internal_order;
        static constexpr std::size_t min_internal_order = std::max(Internal_order / 2, 1UL);
        static constexpr std::size_t leaf_order = Leaf_order;
        static constexpr std::size_t min_leaf_order = std::max(Leaf_order / 2, 1UL);
        template<bool Is_leaf>
        static consteval  std::size_t get_order() {
            if constexpr  (Is_leaf) return leaf_order; else return internal_order;
        }
        template<bool Is_leaf>
        static consteval std::size_t get_min_order() {
            if constexpr (Is_leaf) return min_leaf_order; else return min_internal_order;
        }

    };

    template<template<typename> typename Node_type
    , typename Key, typename Value, typename Index,
        std::size_t Page_size, std::size_t Min_order = 1, std::size_t Max_order = 1024>
    constexpr std::size_t best_order() {
        constexpr std::size_t order = std::midpoint(Min_order, Max_order);
        using traits = traits_type<Key, Value, Index, order, order>;
        std::size_t const size = sizeof(Node_type<traits>);
        if constexpr (size > Page_size)
            return best_order<Node_type, Key, Value, Index, Page_size, Min_order, order>();
        if constexpr (Min_order < Max_order - 1 && size < Page_size)
            return best_order<Node_type, Key, Value, Index, Page_size, order, Max_order>();
        return order;
    }

    template<typename Btree_traits>
    class btree_iterator_base {
    public:
        using key_type = typename Btree_traits::key_type;
        using value_type = typename Btree_traits::value_type;
        using index_type = typename Btree_traits::index_type;
        using this_type = btree_iterator_base;
        using btree_type = btree<key_type, value_type, index_type, Btree_traits::internal_order, Btree_traits::leaf_order>;
        using leaf_node_type = typename btree_type::leaf_node_type;
        using internal_node_type = typename btree_type::internal_node_type;
        using common_node_type = typename btree_type::common_node_type;

        static constexpr index_type INVALID_INDEX = internal_node_type::INVALID_INDEX;

        explicit btree_iterator_base(btree_type *btree = nullptr,
                                     const index_type &leaf_node_index = INVALID_INDEX,
                                     const index_type &leaf_index = 0)
            : btree_(btree),
              leaf_node_index_(leaf_node_index),
              leaf_index_(leaf_index) {
            if (leaf_node_index == INVALID_INDEX)
                set_end();
        }

        btree_iterator_base(const btree_iterator_base &other)
            : btree_(other.btree_),
              leaf_node_index_(other.leaf_node_index_),
              leaf_index_(other.leaf_index_) {
        }

        btree_iterator_base(btree_iterator_base &&other) noexcept
            : btree_(other.btree_),
              leaf_node_index_(std::move(other.leaf_node_index_)),
              leaf_index_(std::move(other.leaf_index_)) {
        }

        btree_iterator_base & operator=(const btree_iterator_base &other) {
            if (this == &other)
                return *this;
            btree_ = other.btree_;
            leaf_node_index_ = other.leaf_node_index_;
            leaf_index_ = other.leaf_index_;
            return *this;
        }

        btree_iterator_base & operator=(btree_iterator_base &&other) noexcept {
            if (this == &other)
                return *this;
            btree_ = other.btree_;
            leaf_node_index_ = std::move(other.leaf_node_index_);
            leaf_index_ = std::move(other.leaf_index_);
            return *this;
        }

        friend bool operator==(const btree_iterator_base &lhs, const btree_iterator_base &rhs) {
            return std::tie(lhs.btree_, lhs.leaf_node_index_, lhs.leaf_index_)
                   == std::tie(rhs.btree_, rhs.leaf_node_index_, rhs.leaf_index_);
        }

    protected:
        auto current_leaf() -> leaf_node_type & {
            return btree_->leaf_node(leaf_node_index_);
        }

        auto current_leaf() const -> leaf_node_type const & {
            return btree_->leaf_node(leaf_node_index_);
        }

        auto incr() -> void {
            auto &node = this->current_leaf();
            ++leaf_index_;
            if (leaf_index_ >= node.keys().size())
                this->go_forward();
        }

        auto decr() -> void {
            if (leaf_index_ == 0)
                go_backward();
            else
                --leaf_index_;
        }

        auto go_forward() -> void {
            if (auto &node = current_leaf(); node.has_next_leaf_index()) {
                leaf_node_index_ = node.next_leaf_index();
                leaf_index_ = 0;
            } else {
                set_end();
            }
        }

        auto go_backward() -> void {
            if (is_end()) {
                leaf_node_index_ = btree_->last_leaf_index();
                leaf_node_type const * p_leaf = &current_leaf();
                leaf_index_ = p_leaf->keys().size() > index_type(0) ? p_leaf->keys().size() - index_type(1) : index_type(0);
            } else if (auto &node = current_leaf(); node.has_previous_leaf_index()) {
                leaf_node_index_ = node.previous_leaf_index();
                auto &prev_node = current_leaf();
                leaf_index_ = prev_node.keys().size() > index_type(0) ? prev_node.keys().size() - index_type(1) : index_type(0);
            } else {
                set_begin();
            }
        }

        auto set_end() -> void {
            *this = btree_->end();
        }

        [[nodiscard]] auto is_end() const -> bool {
            return (*this) == btree_->end();
        }

        auto set_begin() -> void {
            leaf_node_index_ = btree_->first_leaf_index();
            leaf_index_ = index_type(0);
        }

    protected:
        friend btree_type;
        friend btree_test_class;

        btree_type *btree_;
        index_type leaf_node_index_ = INVALID_INDEX;
        index_type leaf_index_ = index_type(0);
    };

    template<typename Btree_traits>
    class btree_iterator : public btree_iterator_base<Btree_traits> {
    public:
        using key_type = typename Btree_traits::key_type;
        using value_type = typename Btree_traits::value_type;
        using index_type = typename Btree_traits::index_type;
        using this_type = btree_iterator;
        using base_type = btree_iterator_base<Btree_traits>;
        using btree_type = btree<key_type, value_type, index_type, Btree_traits::internal_order, Btree_traits::leaf_order>;
        using leaf_node_type = typename btree_type::leaf_node_type;
        using internal_node_type = typename btree_type::internal_node_type;
        using common_node_type = typename btree_type::common_node_type;

        static constexpr index_type INVALID_INDEX = internal_node_type::INVALID_INDEX;

        btree_iterator() = default;

        explicit btree_iterator(btree_type &btree, const index_type &leaf_node_index = INVALID_INDEX, const index_type &leaf_index = index_type(0))
            : base_type(&btree, leaf_node_index, leaf_index) {
        }



        btree_iterator & operator++() {
            this->incr();
            return *this;
        }

        btree_iterator operator++(int) {
            btree_iterator tmp = *this;
            this->incr();
            return tmp;
        }

        btree_iterator & operator--() {
            this->decr();
            return *this;
        }

        btree_iterator operator--(int) {
            btree_iterator tmp = *this;
            this->decr();
            return tmp;
        }

        auto operator*() /*-> std::pair<std::reference_wrapper<key_type>, std::reference_wrapper<value_type>>*/ {
            auto& node = this->current_leaf();
            assert((this->leaf_index_ < node.keys().size()) && "key index out of bounds" );
            assert((this->leaf_index_ < node.values().size()) && "value index out of bounds");
            return std::make_pair(
                std::cref(node.keys()[this->leaf_index_]), std::ref(node.values()[this->leaf_index_])
                );
        };

    private:
        friend btree_type;
        friend btree_test_class;
    };

    template<typename Btree_traits>
    class btree_const_iterator : public btree_iterator_base<Btree_traits> {
    public:
        using key_type = typename Btree_traits::key_type;
        using value_type = typename Btree_traits::value_type;
        using index_type = typename Btree_traits::index_type;
        using this_type = btree_const_iterator;
        using base_type = btree_iterator_base<Btree_traits>;
        using btree_type = btree<key_type, value_type, index_type, Btree_traits::internal_order, Btree_traits::leaf_order>;
        using leaf_node_type = typename btree_type::leaf_node_type;
        using internal_node_type = typename btree_type::internal_node_type;
        using common_node_type = typename btree_type::common_node_type;

        static constexpr index_type INVALID_INDEX = internal_node_type::INVALID_INDEX;

        btree_const_iterator() = default;

        explicit btree_const_iterator(btree_type const &btree, const index_type &leaf_node_index = INVALID_INDEX, const index_type &leaf_index = index_type(0))
            : base_type(const_cast<btree_type*>(&btree), leaf_node_index, leaf_index) {
        }

        explicit btree_const_iterator(btree_iterator<Btree_traits> const &other)
            : base_type(other)
        {}

        btree_const_iterator & operator++() {
            this->incr();
            return *this;
        }

        btree_const_iterator operator++(int) {
            btree_const_iterator tmp = *this;
            this->incr();
            return tmp;
        }

        btree_const_iterator & operator--() {
            this->decr();
            return *this;
        }

        btree_const_iterator operator--(int) {
            btree_const_iterator tmp = *this;
            this->decr();
            return tmp;
        }

        auto operator*() const /*-> std::pair<std::reference_wrapper<key_type>, std::reference_wrapper<value_type>>*/ {
            auto& node = this->current_leaf();
            assert((this->leaf_index_ < node.keys().size()) && "key index out of bounds");
            assert((this->leaf_index_ < node.values().size()) && "value index out of bounds");
            return std::make_pair(
                std::cref(node.keys()[this->leaf_index_]), std::cref(node.values()[this->leaf_index_])
                );
        };

    private:
        friend btree_type;
        friend btree_test_class;
    };

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    class btree {
    public:
        static_assert(std::numeric_limits<Index>::max() > Internal_order + 2); // + 2 for distance to end() of child_indices
        static_assert(std::numeric_limits<Index>::max() > Leaf_order + 1); // + 1 for distance to end()
        using traits = traits_type<Key, Value, Index, Internal_order, Leaf_order>;
        using key_type = typename traits::key_type;
        using value_type = typename traits::value_type;
        using index_type = typename traits::index_type;
        using this_type = btree;
        using internal_node_type = btree_internal_node<traits>;
        using leaf_node_type = btree_leaf_node<traits>;
        using common_node_type = std::variant<internal_node_type, leaf_node_type>;

        using iterator_base_type = btree_iterator_base<traits>;
        using iterator = btree_iterator<traits>;
        using const_iterator = btree_const_iterator<traits>;

        static constexpr index_type INVALID_INDEX = internal_node_type::INVALID_INDEX;

        btree() = default;

        btree(const btree &other)
            : nodes_(other.nodes_),
              root_index_(other.root_index_) {
        }

        btree(btree &&other) noexcept
            : nodes_(std::move(other.nodes_)),
              root_index_(std::move(other.root_index_)) {
        }

        btree & operator=(const btree &other) {
            if (this == &other)
                return *this;
            nodes_ = other.nodes_;
            root_index_ = other.root_index_;
            return *this;
        }

        btree & operator=(btree &&other) noexcept {
            if (this == &other)
                return *this;
            nodes_ = std::move(other.nodes_);
            root_index_ = std::move(other.root_index_);
            return *this;
        }

        friend bool operator==(const btree &lhs, const btree &rhs) {
            auto itl = lhs.begin();
            auto el = lhs.end();
            auto itr = rhs.begin();
            auto er = rhs.end();
            bool equal = true;
            while (equal && itl != el && itr != er) {
                auto const & [lk, lv] = *itl;
                auto const & [rk, rv] = *itr;
                equal = (lk == rk) && (lv == rv);
                ++itl;
                ++itr;
            }
            return equal && itl == el && itr == er;
        }

        friend bool operator!=(const btree &lhs, const btree &rhs) {
            return !(lhs == rhs);
        }

        auto begin() -> iterator {
            auto index = first_leaf_index();
            return iterator(*this, index);
        }
        auto begin() const -> const_iterator {
            auto index = first_leaf_index();
            return const_iterator(*this, index);
        }
        auto cbegin() const -> const_iterator { return begin(); };

        auto end() -> iterator {
            auto index = last_leaf_index();
            leaf_node_type const & leaf = leaf_node(index);
            index_type last = leaf.size();
            return iterator(*this, index, last);
        }
        auto end() const -> const_iterator {
            auto index = last_leaf_index();
            leaf_node_type const & leaf = leaf_node(index);
            index_type last = leaf.size();
            return const_iterator(*this, index, last);
        }
        auto cend() const -> const_iterator { return end(); }

        auto insert(key_type const &key, value_type const &value) -> bool ;

        // TODO: implement
        auto erase(key_type const& key) -> std::size_t;
        auto erase(iterator it) -> std::size_t;
        auto erase(const_iterator first, const_iterator last) -> std::size_t;

        auto find(key_type const& key) -> iterator;
        auto find(key_type const& key) const -> const_iterator;

        auto find_last(key_type const& key) -> iterator;

        auto contains(key_type const &key) const -> bool { return find(key) != end(); }

        // TODO: implement
        auto get(const key_type &key) const -> const value_type&;
        auto get_or(const key_type &key, const value_type &default_value = value_type()) -> value_type const&;

        friend std::ostream &operator<<(std::ostream &out, const btree &tree) {
            auto out_array = [](std::ostream &o, auto const &a, std::string const &separator = ", ") -> std::ostream& {
                if (a.size() > 0)
                    o << a[0];
                for (index_type i = 1; i < a.size(); ++i)
                    o << ", " << a[i];
                return o;
            };
            std::function<void(const common_node_type&)> stringify = [&tree, &out, &stringify, &out_array](auto const &node) -> void {
                std::visit(
                    [&tree, &out, &stringify, &out_array](auto const &this_node) {
                        auto d = tree.node_depth(this_node.index_);
                        std::string prefix(static_cast<size_t>(4 * d), ' ');
                        out << prefix << "\"" << this_node.index() << "\": {\n";
                        out << prefix << "  \"parent\": " << this_node.parent_index() << ",\n";
                        out << prefix << "  \"keys\": [";
                        out_array(out, this_node.keys())  << "], \n";;
                        if constexpr (std::is_same_v<std::decay_t<decltype(this_node)>, leaf_node_type>) {
                            out << prefix << "  \"values\": [";
                            out_array(out, this_node.values())  << "],\n";
                            out << prefix << "  \"previous\": " << this_node.previous_leaf_index() << ",\n";
                            out << prefix << "  \"next\": " << this_node.next_leaf_index() << "}\n";
                        } else {
                            out << prefix << "  \"children\": {\n";
                            for (index_type i = 0; i < this_node.child_indices().size(); ++i)
                                stringify(tree.node(this_node.child_indices()[i]));
                            out << prefix << "  }\n" << prefix << "},\n";
                        }
                    },
                    node);
            };
            out << "{\n";
            stringify(tree.node(tree.root_index()));
            out << "}\n";
            return out;
        }

        [[nodiscard]] explicit operator std::string() const {
            std::ostringstream oss;
            oss << (*this);
            return oss.str();
        }

#ifndef BTREE_TESTING
    protected:
#endif
        auto is_root(index_type index) const noexcept -> bool {
            return index == root_index_;
        }
        auto is_root(leaf_node_type const & node) const noexcept -> bool {
            return is_root(node.index());
        }
        auto is_root(internal_node_type const & node) const noexcept -> bool {
            return is_root(node.index());
        }
        [[nodiscard]] index_type root_index() const { return root_index_; }

        [[nodiscard]] auto node_depth(index_type node_index) const -> index_type {
            return std::visit([this](auto& n) -> index_type {
                if (n.has_parent())
                    return index_type(1) + node_depth(n.parent_index());
                return index_type(1);
            }, node(node_index));
        }

        auto node(index_type const & index) -> common_node_type& {
            assert((index < nodes_.size()) && "node index out of bounds");
            if (index >= nodes_.size())
                throw std::out_of_range("node(index_type const & index): node index out of bounds");
            return nodes_[index];
        }
        auto node(index_type const & index) const -> const common_node_type& {
            assert((index < nodes_.size()) && "node index out of bounds");
            if (index >= nodes_.size())
                throw std::out_of_range("node(index_type const & index): node index out of bounds");
            return nodes_[index];
        }

        auto leaf_node(index_type const & index) -> leaf_node_type& {
            common_node_type& r_node = node(index);
            assert((std::holds_alternative<leaf_node_type>(r_node)) && "index is not a leaf node");
            leaf_node_type* p_leaf = std::get_if<leaf_node_type>(&r_node);
            if (p_leaf == nullptr)
                throw std::runtime_error("leaf_node(index_type const & index): index does no denote a leaf node");
            return *p_leaf;
        }

        auto leaf_node(index_type const &index) const -> leaf_node_type const & {
            common_node_type const &r_node = node(index);
            assert((std::holds_alternative<leaf_node_type>(r_node)) && "index is not a leaf node");
            leaf_node_type const *p_leaf = std::get_if<leaf_node_type>(&r_node);
            if (p_leaf == nullptr)
                throw std::runtime_error("leaf_node(index_type const &index): index does no denote a leaf node");
            return *p_leaf;
        }

        auto internal_node(index_type const & index) -> internal_node_type& {
            common_node_type& r_node = node(index);
            assert((std::holds_alternative<internal_node_type>(r_node)) && "index is not an internal node");
            internal_node_type* p_internal = std::get_if<internal_node_type>(&r_node);
            if (p_internal == nullptr)
                throw std::runtime_error("internal_node(index_type const & index): index does no denote a internal node");
            return *p_internal;
        }

        auto internal_node(index_type const & index) const -> internal_node_type const & {
            common_node_type const & r_node = node(index);
            assert((std::holds_alternative<internal_node_type>(r_node)) && "index is not an internal node");
            internal_node_type const * p_internal = std::get_if<internal_node_type>(&r_node);
            if (p_internal == nullptr)
                throw std::runtime_error("internal_node(index_type const & index): index does no denote a internal node");
            return *p_internal;
        }

        auto minimum_key(index_type index) -> key_type const &;

        /**
         * @brief Create a new root node
         * @param left_index the index of the left child node (which is the current root)
         * @param right_index the index of the right child node
         * @param pivot_key
         * @return the new root index
         */
        auto grow(index_type left_index, index_type right_index, key_type const &pivot_key) -> index_type;

        auto shrink() -> index_type;

        auto create_internal_node(index_type const &parent_index = INVALID_INDEX) -> index_type;

        auto create_leaf_node(index_type const &parent_index = INVALID_INDEX) -> index_type;

        auto first_leaf_index() const -> index_type;

        auto last_leaf_index() const -> index_type;

        auto find_insert_position(const key_type &key, const index_type &start_index) -> iterator;

        auto find_first(key_type const& key) const -> std::tuple<index_type, index_type>;

        auto insert_split_internal(index_type node_index, const key_type &key, index_type child_index) -> bool;

        auto insert_internal(index_type node_index, const key_type &key, index_type child_index, bool allow_recurse) -> bool;

        auto insert_split_leaf(iterator insert_pos, const key_type &key, const value_type &value) -> bool;

        auto insert_leaf(iterator insert_pos, const key_type &key, const value_type &value, bool allow_recurse) -> bool;

        auto merge_internal(index_type left_node_index) -> bool;

        auto erase_internal(index_type internal_node_index, index_type child_node_index) -> bool;

        auto merge_leaf(index_type left_leaf_index) -> bool;

        auto rebalance_internal_node(index_type internal_node_index) -> bool;

        auto rebalance_leaf_node(index_type leaf_nodex_index) -> bool;

        auto delete_node(index_type node_index);

        /**
         * @brief For the node with child_node_index set the correlated key in the (internal) parent node to the first value of the child node
         * @param child_node_index
         * @param p_correlated_key a pointer to the first key of the child_node used to transport it further up the tree
         */
        auto adjust_parent_key(index_type child_node_index, key_type const *p_correlated_key = nullptr) -> void;

#ifndef BTREE_TESTING
    private:
#endif
        friend iterator_base_type;
        friend iterator;
        friend const_iterator;
        friend btree_test_class;

        std::vector<common_node_type> nodes_{leaf_node_type{0, INVALID_INDEX}};
        index_type root_index_{0};
    };

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::insert(key_type const &key, value_type const &value) -> bool {
        iterator it = find_insert_position(key, root_index());
        return insert_leaf(it, key, value, true);
    }

    // template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    // auto btree<Key, Value, Index, Internal_order, Leaf_order>::erase(key_type const &key) -> std::size_t {
    //     return 0; // TODO
    // }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::erase(iterator it) -> std::size_t {
        leaf_node_type& leaf = it.current_leaf();
        assert((leaf.size() > 0) && "erase(const_iterator it): leaf is empty");
        auto erase_key_it = leaf.keys().begin() + it.leaf_index_;
        leaf.keys().erase(erase_key_it);
        leaf.values().erase(leaf.values().begin() + it.leaf_index_);
        if (erase_key_it == leaf.keys().begin() && !is_root(leaf.index())) {
            adjust_parent_key(leaf.index());
        }
        if (leaf.size() < traits::template get_min_order<true>()) {
            rebalance_leaf_node(it.leaf_node_index_);
        }
        return 1;
    }

    // template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    // auto btree<Key, Value, Index, Internal_order, Leaf_order>::erase(const_iterator first,
    //     const_iterator last) -> std::size_t {
    //     return 0; // TODO
    // }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::find(key_type const &key) -> iterator {
        auto [leaf_node_index, leaf_index] = find_first(key);
        return iterator(*this, leaf_node_index, leaf_index);
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::find(key_type const &key) const -> const_iterator {
        auto [leaf_node_index, leaf_index] = find_first(key);
        return const_iterator(*this, leaf_node_index, leaf_index);
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::find_last(key_type const &key) -> iterator {
        index_type index = root_index();
        for (const internal_node_type *p_node = std::get_if<internal_node_type>(&node(index));
             p_node != nullptr;
             p_node = std::get_if<internal_node_type>(&node(index))) {
            auto it = std::ranges::upper_bound(p_node->keys(), key);
            auto dist = std::distance(p_node->keys().begin(), it);
            index = p_node->child_indices().at(dist);
        }
        leaf_node_type& leaf = leaf_node(index);
        auto it = std::ranges::upper_bound(leaf.keys(), key);
        it = std::ranges::prev(it, 1, leaf.keys().begin());
        if (key < *it)
            return end();
        return iterator(*this, index, index_type(std::distance(leaf.keys().begin(), it)));
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::minimum_key(index_type index) -> key_type const & {
        return std::visit([this](auto const & node) -> decltype(auto) {
            if constexpr (std::is_same_v<std::decay_t<decltype(node)>, leaf_node_type>) {
                return node.keys().front();
            } else {
                return minimum_key(node.child_indices().front());
            }
        }, node(index));
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::grow(index_type left_index,
                                               index_type right_index, key_type const &pivot_key) -> index_type {
        assert((is_root(left_index)) && "left node ist supposed the be the old root");
        auto new_root_index = create_internal_node(INVALID_INDEX);
        internal_node_type& new_root = internal_node(new_root_index);
        new_root.child_indices().push_back(left_index);
        new_root.child_indices().push_back(right_index);

        new_root.keys().push_back(pivot_key);

        for(auto child_index : {left_index, right_index}) {
            std::visit([new_root_index](auto & node) {
                node.set_parent_index(new_root_index);
            }, node(child_index));
        }

        root_index_ = new_root_index;

        return new_root_index;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::shrink() -> index_type {
        auto& root_node = node(root_index());
        internal_node_type* p_old_root = std::get_if<internal_node_type>(&root_node);
        // assert((p_old_root != nullptr) && "Cannot shrink with leaf root node");
        if (p_old_root == nullptr)
            throw std::runtime_error("Cannot shrink with leaf root node");
        assert((p_old_root->child_indices().size() == 1) && "shrink(): root node has more or less than 1 child");
        root_index_ = p_old_root->child_indices().front();
        p_old_root->mark_deleted();
        auto& new_root_node = node(root_index());
        std::visit([](auto & node) {
            node.set_parent_index(INVALID_INDEX);
        }, new_root_node);
        return root_index();
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::create_internal_node(index_type const &parent_index) -> index_type {
        auto index = index_type(nodes_.size());
        assert((index != INVALID_INDEX) && "create_internal_node: node index overflow");
        nodes_.emplace_back(std::move(internal_node_type(index, parent_index)));
        return index;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::create_leaf_node(index_type const &parent_index) -> index_type {
        auto index = index_type(nodes_.size());
        assert((index != INVALID_INDEX) && "create_leaf_node: node index overflow");
        nodes_.emplace_back(std::move(leaf_node_type(index, parent_index)));
        return index;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::first_leaf_index() const -> index_type {
        auto index = root_index_;
        for (const internal_node_type *p_node = std::get_if<internal_node_type>(&node(index));
            p_node != nullptr;
             index = p_node->child_indices().front(), p_node = std::get_if<internal_node_type>(&node(index)))
            {}
        return index;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::last_leaf_index() const -> index_type {
        auto index = root_index_;
        for (const internal_node_type *p_node = std::get_if<internal_node_type>(&node(index));
            p_node != nullptr;
            index = p_node->child_indices().back(), p_node = std::get_if<internal_node_type>(&node(index)))
            {}
        return index;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::find_insert_position(const key_type &key, const index_type &start_index) -> iterator {
        index_type node_index = start_index;
        do {
            if (node_index == INVALID_INDEX) {
                assert((false) && "find_insert_position(const key_type &key, const index_type &start_index): should never happen");
                return end();
            }
            common_node_type *p_node = &node(node_index);
            auto result = std::visit([&](auto &node) -> std::variant<index_type, iterator> {
                auto found = std::ranges::upper_bound(node.keys(), key); // found > key
                index_type found_index = index_type(std::distance(node.keys().begin(), found));
                if constexpr (std::is_same_v<std::decay_t<decltype(node)>, internal_node_type>) {
                    node_index = node.child_indices_[found_index];
                    return node_index;
                } else {
                    static_assert(std::is_same_v<std::decay_t<decltype(node)>, leaf_node_type>);
                    return iterator(*this, node_index, found_index);
                }
            }, *p_node);
            if (std::holds_alternative<iterator>(result))
                return std::get<iterator>(result);
        } while (true);
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::find_first(key_type const &key) const -> std::tuple<index_type, index_type> {
        index_type index = root_index();
        for (const internal_node_type *p_node = std::get_if<internal_node_type>(&node(index));
             p_node != nullptr;
             p_node = std::get_if<internal_node_type>(&node(index))) {
            auto it = std::ranges::lower_bound(p_node->keys(), key);
            auto dist = std::distance(p_node->keys().begin(), it);
            if (it != p_node->keys().end() && *it == key)
                ++dist;
            index = p_node->child_indices().at(static_cast<size_t>(dist));
        }
        leaf_node_type const & leaf = leaf_node(index);
        auto it = std::ranges::lower_bound(leaf.keys(), key);
        if (it == leaf.keys().end() || key != *it)
            return std::make_tuple(INVALID_INDEX, 0);
        return std::make_tuple(index, std::distance(leaf.keys().begin(), it));
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::insert_split_internal(index_type node_index, const key_type &key,
        index_type child_index) -> bool {
        assert((internal_node(node_index).size() == internal_node_type::order()) && "internal node should be full");

        // create new internal
        index_type new_internal_index = create_internal_node(internal_node(node_index).parent_index());
        internal_node_type& new_internal = internal_node(new_internal_index);

        internal_node_type* p_internal = &internal_node(node_index);

        auto pivot_key_it = std::midpoint(p_internal->keys().begin(), p_internal->keys().end());
        auto pivot_index = std::distance(p_internal->keys().begin(), pivot_key_it);

        auto key_insert_it = std::ranges::upper_bound(p_internal->keys(), key);
        auto key_insert_index = std::distance(p_internal->keys().begin(), key_insert_it);

        // save pivot
        key_type pivot_key = *pivot_key_it;
        bool insert_new_key_left = key_insert_index < pivot_index;
        bool insert_new_key_right = pivot_index < key_insert_index;
        // TODO: first find where to insert new key, then decide whether to split
        //       on the pivot key or after
        internal_node_type* p_insert_internal = insert_new_key_left ? p_internal : &new_internal;

        // move keys/child_indices which are right from pivot into new node
        auto first_key_it = pivot_key_it;
        auto first_child_indices_it = p_internal->child_indices().begin() + pivot_index;

        auto advance = insert_new_key_left ? 0 : 1;
        std::ranges::advance(first_key_it, advance, p_internal->keys().end());
        std::move(first_key_it, p_internal->keys().end(), std::back_inserter(new_internal.keys()));

        std::ranges::advance(first_child_indices_it, advance, p_internal->child_indices().end());
        std::move(first_child_indices_it, p_internal->child_indices().end(), std::back_inserter(new_internal.child_indices()));

        // shrink left node
        // p_internal->keys().resize(pivot_index + advance - 1);
        auto remove_from = first_key_it;
        std::ranges::advance(remove_from, -1, p_internal->keys().begin());
        p_internal->keys().erase(remove_from, p_internal->keys().end());
        // p_internal->child_indices().resize(pivot_index + advance);
        p_internal->child_indices().erase(first_child_indices_it, p_internal->child_indices().end());

        if (!(insert_new_key_left || insert_new_key_right)) {
            // new key == pivot key
            pivot_key = key;
            index_type last_first_child_index = new_internal.child_indices().front();
            new_internal.child_indices().insert(new_internal.child_indices().begin(), child_index);
            std::visit([&](auto &node) {
               node.set_parent_index(new_internal_index);
            }, node(child_index));
            auto&& min_key = minimum_key(last_first_child_index);
            new_internal.keys().insert(new_internal.keys().begin(), std::move(min_key));
        }
        // adjust parent index for all children of the new right node
        for (auto index : new_internal.child_indices())
            std::visit([&](auto &node) {
               node.set_parent_index(new_internal_index);
            }, node(index));

        if (insert_new_key_left || insert_new_key_right) {
            // insert key and value into one of the internal nodes
            insert_internal(p_insert_internal->index(), key, child_index, false);
        }

        pivot_key = minimum_key(new_internal_index);
        if (is_root(*p_internal)) {
            // pivot_key = minimum_key(new_internal_index);
            grow(node_index, new_internal_index, pivot_key);
        } else {
            insert_internal(p_internal->parent_index(), pivot_key, new_internal_index, true);
        }

        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::insert_internal(index_type node_index, const key_type &key,
                                                          index_type child_index, bool allow_recurse) -> bool {
        internal_node_type& internal = internal_node(node_index);
        if (internal.size() < internal.order()) {
            auto insert_pos_it = std::upper_bound(internal.keys().begin(), internal.keys().end(), key);
            auto insert_pos_index = std::distance(internal.keys().begin(), insert_pos_it);
            internal.keys().insert(insert_pos_it, key);
            internal.child_indices().insert(internal.child_indices().begin() + insert_pos_index + 1, child_index);
            std::visit([node_index](auto & child_node) {
                child_node.set_parent_index(node_index);
            }, node(child_index));
            // if (insert_pos_index == index_type(0))
            //     adjust_parent_key(node_index);
        } else {
            assert((allow_recurse == true) && "already recursed into insert_internal");
            insert_split_internal(node_index, key, child_index);
        }
        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::insert_split_leaf(iterator insert_pos, const key_type &key, const value_type &value)-> bool {
        assert((insert_pos.current_leaf().keys().size() == insert_pos.current_leaf().keys().capacity()) && "leaf node should be full");

        // create a new leaf
        index_type new_leaf_index = create_leaf_node(insert_pos.current_leaf().parent_index());
        leaf_node_type& new_leaf = leaf_node(new_leaf_index);

        // p_leaf
        leaf_node_type* p_leaf = &insert_pos.current_leaf();

        auto pivot_key_it = std::midpoint(p_leaf->keys().begin(), p_leaf->keys().end());
        auto pivot_index = std::distance(p_leaf->keys().begin(), pivot_key_it);
        auto pivot_value_it = p_leaf->values().begin() + pivot_index;

        // save pivot
        key_type pivot_key = *pivot_key_it;

        // move pivot keys/values and everything right into new node
        std::move(pivot_key_it, p_leaf->keys().end(), std::back_inserter(new_leaf.keys()));
        std::move(pivot_value_it, p_leaf->values().end(), std::back_inserter(new_leaf.values()));

        // adjust links
        new_leaf.set_next_leaf_index(p_leaf->next_leaf_index());
        new_leaf.set_previous_leaf_index(p_leaf->index());
        p_leaf->set_next_leaf_index(new_leaf_index);
        if (new_leaf.has_next_leaf_index()) {
            auto& next_leaf = leaf_node(new_leaf.next_leaf_index());
            next_leaf.set_previous_leaf_index(new_leaf.index());
        }

        // shrink left node
        p_leaf->keys().resize(pivot_index);
        p_leaf->values().resize(pivot_index);

        // insert key and value into one of the leaf nodes
        leaf_node_type* p_insert_leaf = (key < pivot_key) ? p_leaf : &new_leaf;
        auto new_insert_pos = find_insert_position(key, p_insert_leaf->index());
        insert_leaf(new_insert_pos, key, value, false);

        if (is_root(*p_leaf)) {
            grow(p_leaf->index(), new_leaf_index, pivot_key);
        } else {
            // insert pivot_key into parent (internal) node
            insert_internal(p_leaf->parent_index(), pivot_key, new_leaf_index, true);
        }

        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::insert_leaf(iterator insert_pos, const key_type &key,
        const value_type &value, bool allow_recurse) -> bool {
        leaf_node_type& leaf = insert_pos.current_leaf();
        if (leaf.size() < leaf_node_type::order()) {
            leaf.keys_.insert(leaf.keys_.begin() + insert_pos.leaf_index_, key);
            leaf.values_.insert(leaf.values_.begin() + insert_pos.leaf_index_, value);
        } else {
            assert((allow_recurse == true) && "already recursed into insert_leaf");
            insert_split_leaf(insert_pos, key, value);
        }
        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::merge_internal(index_type left_node_index) -> bool {
        assert(!is_root(left_node_index) && "merge_internal(index_type left_node_index): Cannot merge root node");
        internal_node_type* p_left = &internal_node(left_node_index);
        // assert((p_left->size() < traits::min_internal_order) && "merge_internal(left_node_index internal_node_index): left node is to big to merge");
        internal_node_type* p_parent = &internal_node(p_left->parent_index());
        auto [_, right_index] = p_parent->siblings_for_index(left_node_index);
        assert((right_index != INVALID_INDEX) && "merge_internal(index_type left_node_index): There is no right node to merge with");
        internal_node_type* p_right = &internal_node(right_index);
        assert((p_left->size() + p_right->size() < traits::internal_order) && "merge_internal(left_node_index internal_node_index): left + right node are to big to merge");

        auto [right_key_it, right_index_it] = p_parent->iterators_for_index(right_index);
        auto key_inserter = std::back_inserter(p_left->keys());
        *key_inserter = std::move(*right_key_it);
        std::move(p_right->keys().begin(), p_right->keys().end(), key_inserter);
        p_right->keys().clear();
        auto index_inserter = std::back_inserter(p_left->child_indices());
        for(auto & i : p_right->child_indices()) {
            *index_inserter = std::move(i);
            std::visit([left_node_index](auto & node) {
                node.set_parent_index(left_node_index);
            }, node(i));
        }
        erase_internal(p_right->parent_index(), right_index);
        p_right->mark_deleted();
        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::erase_internal(index_type internal_node_index,
        index_type child_node_index) -> bool {
        internal_node_type &internal = internal_node(internal_node_index);
        auto [key_it, index_it] = internal.iterators_for_index(child_node_index);
        assert((*index_it == child_node_index) && "erase_internal: Error finding child index");
        if (key_it == internal.keys().end())
            key_it = internal.keys().begin();
        internal.child_indices().erase(index_it);
        internal.keys().erase(key_it);
        if (internal.size() < traits::min_internal_order)
            rebalance_internal_node(internal_node_index);
        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::merge_leaf(index_type left_leaf_index) -> bool {
        // merge with the neighbour with the same parent node as this_node
        //         - move all key/values to the lesser node
        //         - adjust previous and next node indexes
        //         - remove keys and index of the greater node from the parent node
        //         - mark right node as deleted/unused
        //         - check if we need to rebalance parent internal node (recurse)
        //         - check if we need to shrink
        leaf_node_type& left_leaf = leaf_node(left_leaf_index);
        assert((left_leaf.has_next_leaf_index() ) && "merge_leaf(index_type left_leaf_index): Left node has no next node");
        auto right_leaf_index = left_leaf.next_leaf_index();
        leaf_node_type& right_leaf = leaf_node(right_leaf_index);
        // assert((left_leaf.parent_index() == right_leaf.parent_index()) && "merge_leaf(index_type left_leaf_index): Cannot merge leaf nodes with different parent nodes");
        assert((right_leaf.has_previous_leaf_index() && right_leaf.previous_leaf_index() == left_leaf_index) && "Right node does not point to left node");
        assert((left_leaf.keys().front() <= right_leaf.keys().front()) && "merge_leaf(index_type left_leaf_index): order of nodes is obviously wrong");
        assert((left_leaf.size() <= traits::min_leaf_order) && "merge_leaf(index_type left_leaf_index): left node is to big to merge");
        assert((right_leaf.size() <= traits::min_leaf_order) && "merge_leaf(index_type left_leaf_index): right node is to big to merge");
        assert((left_leaf.size() + right_leaf.size() <= traits::leaf_order) && "merge_leaf(index_type left_leaf_index): sizes of nodes to big to merge");

        std::move(right_leaf.keys().begin(), right_leaf.keys().end(), std::back_inserter(left_leaf.keys()));
        right_leaf.keys().clear();
        std::move(right_leaf.values().begin(), right_leaf.values().end(), std::back_inserter(left_leaf.values()));
        right_leaf.values().clear();

        left_leaf.set_next_leaf_index(right_leaf.next_leaf_index());
        erase_internal(right_leaf.parent_index(), right_leaf_index);
        if (right_leaf.has_next_leaf_index()) {
            auto& next_leaf = leaf_node(right_leaf.next_leaf_index());
            adjust_parent_key(right_leaf.next_leaf_index());
            next_leaf.set_previous_leaf_index(left_leaf.index());
        }
        right_leaf.mark_deleted();
        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::rebalance_internal_node(index_type internal_node_index) -> bool {
        internal_node_type* p_internal = &internal_node(internal_node_index);
        assert((p_internal->size() < traits::min_internal_order) && "rebalance_internal_node: left node has sufficient keys already");
        if (is_root(internal_node_index)) {
            if (p_internal->size() == 0) {
                shrink();
                return true;
            }
            return false;
        }
        internal_node_type* p_parent = &internal_node(p_internal->parent_index());
        auto [prev_index, next_index] = p_parent->siblings_for_index(internal_node_index);

        internal_node_type* p_prev = nullptr;
        index_type prev_size = 0;
        if (prev_index != INVALID_INDEX) {
            p_prev = &internal_node(prev_index);
            prev_size = p_prev->size();
        }

        internal_node_type* p_next = nullptr;
        index_type next_size = 0;
        if (next_index != INVALID_INDEX) {
            p_next = &internal_node(next_index);
            next_size = p_next->size();
        }

        internal_node_type* p_chosen_neighbour = nullptr;
        switch(0x02 * (prev_size > traits::min_internal_order) + (next_size > traits::min_internal_order)) {
            case 0x03: // both
                p_chosen_neighbour = prev_size > next_size ? p_prev : p_next;
                break;
            case 0x02: // prev
                p_chosen_neighbour = p_prev;
                break;
            case 0x01: // next
                p_chosen_neighbour = p_next;
                break;
            case 0x00: // none: merge
                merge_internal(next_index != INVALID_INDEX ? internal_node_index : prev_index);
                return true;
                break;
            default:
                assert("rebalance_internal_node(index_type internal_nodex_index): Unreachable rebalance strategy");
        }
        if (p_chosen_neighbour != nullptr) {
            index_type copy_cnt = std::min(index_type(1), std::midpoint(p_internal->size(), p_chosen_neighbour->size()));
            bool is_next = p_chosen_neighbour == p_next;
            index_type key_start_index = is_next ? 0 : p_chosen_neighbour->size() - copy_cnt;
            index_type value_start_index = is_next ? 0 : p_chosen_neighbour->child_indices().size() - copy_cnt;
            index_type key_end_index = is_next ? copy_cnt : p_chosen_neighbour->size();
            index_type value_end_index = is_next ? copy_cnt : p_chosen_neighbour->child_indices().size();
            auto key_insertion_it = is_next ? p_internal->keys().end() : p_internal->keys().begin();
            auto index_insertion_it = is_next ? p_internal->child_indices().end() : p_internal->child_indices().begin();
            if (is_next) {
                p_internal->keys().resize(p_internal->keys().size() + copy_cnt);
                p_internal->child_indices().resize(p_internal->child_indices().size() + copy_cnt);
            } else {
                p_internal->keys().insert_space(p_internal->keys().begin(), copy_cnt);
                p_internal->child_indices().insert_space(p_internal->child_indices().begin(), copy_cnt);
            }


            std::move(p_chosen_neighbour->keys().begin() + key_start_index, p_chosen_neighbour->keys().begin() + key_end_index, key_insertion_it);
            p_chosen_neighbour->keys().erase(p_chosen_neighbour->keys().begin() + key_start_index, p_chosen_neighbour->keys().begin() + key_end_index);
            std::move(p_chosen_neighbour->child_indices().begin() + value_start_index, p_chosen_neighbour->child_indices().begin() + value_end_index, index_insertion_it);
            p_chosen_neighbour->child_indices().erase(p_chosen_neighbour->child_indices().begin() + value_start_index, p_chosen_neighbour->child_indices().begin() + value_end_index);

            std::for_each(index_insertion_it, index_insertion_it + copy_cnt,
                          [this, internal_node_index](auto index) {
                              std::visit([this, internal_node_index, index](auto &node) {
                                  node.set_parent_index(internal_node_index);
                                  adjust_parent_key(index);
                              }, node(index));
                          }
            );
            if (!is_next)
                adjust_parent_key(p_internal->child_indices().at(copy_cnt));

            adjust_parent_key(p_chosen_neighbour->index());
            // adjust_parent_key(internal_node_index);
        }
        return false;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order,
        Leaf_order>::rebalance_leaf_node(index_type leaf_node_index) -> bool {
        if (is_root(leaf_node_index))
            return false;
        leaf_node_type *p_leaf = &leaf_node(leaf_node_index);
        leaf_node_type *p_next_leaf = nullptr;
        index_type next_size = index_type(0);
        leaf_node_type *p_prev_leaf = nullptr;
        index_type prev_size = index_type(0);
        if (p_leaf->has_next_leaf_index()) {
            p_next_leaf = &leaf_node(p_leaf->next_leaf_index());
            next_size = p_next_leaf->size();
        }
        if (p_leaf->has_previous_leaf_index()) {
            p_prev_leaf = &leaf_node(p_leaf->previous_leaf_index());
            prev_size = p_prev_leaf->size();
        }
        // redistribute keys or merge?
        //           this_leaf.size() < min_order
        //    left.size() > min_order?            right.size() > min_order?
        //  both:  pick neighbour with more nodes -> chosen_neighbour
        //  left:  left -> chosen_neighbour
        //  right: right -> chosen_neighbour
        //         redistribute such that this_leaf and chosen_neighbour have same size +- 1
        //         i.e. move (chosen_neighbour.size() - min_order) / 2 nodes into this_node
        //  none:  merge with the neighbour with the same parent node as this_node

        leaf_node_type* p_chosen_neighbour = nullptr;
        switch(0x02 * (prev_size > traits::min_leaf_order) + (next_size > traits::min_leaf_order)) {
            case 0x03: // both
                p_chosen_neighbour = prev_size > next_size ? p_prev_leaf : p_next_leaf;
                break;
            case 0x02: // prev
                p_chosen_neighbour = p_prev_leaf;
                break;
            case 0x01: // next
                p_chosen_neighbour = p_next_leaf;
                break;
            case 0x00: // none: merge
                merge_leaf(p_leaf->has_next_leaf_index() ? p_leaf->index() : p_leaf->previous_leaf_index() );
                return true;
                break;
            default:
                assert("rebalance_leaf_node(index_type leaf_nodex_index): Unreachable rebalance strategy");
        }
        if (p_chosen_neighbour != nullptr) {
            index_type copy_cnt = std::min(index_type(1), std::midpoint(p_leaf->size(), p_chosen_neighbour->size()));
            bool is_next = p_chosen_neighbour == p_next_leaf;
            index_type start_index = is_next ? 0 : p_chosen_neighbour->size() - copy_cnt;
            index_type end_index = is_next ? copy_cnt : p_chosen_neighbour->size();
            auto key_insertion_it = is_next ? p_leaf->keys().end() : p_leaf->keys().begin();
            auto value_insertion_it = is_next ? p_leaf->values().end() : p_leaf->values().begin();
            if (is_next) {
                // move from beginning of right (p_chosen_neighbour) to end of left (p_leaf)
                p_leaf->keys().resize(p_leaf->keys().size() + copy_cnt);
                p_leaf->values().resize(p_leaf->values().size() + copy_cnt);
            } else {
                // move from end of left (p_chosen_neighbour) to beginning of right (p_leaf)
                p_leaf->keys().insert_space(p_leaf->keys().begin(), copy_cnt);
                p_leaf->values().insert_space(p_leaf->values().begin(), copy_cnt);
            }
            std::move(p_chosen_neighbour->keys().begin() + start_index, p_chosen_neighbour->keys().begin() + end_index, key_insertion_it);
            p_chosen_neighbour->keys().erase(p_chosen_neighbour->keys().begin() + start_index, p_chosen_neighbour->keys().begin() + end_index);
            std::move(p_chosen_neighbour->values().begin() + start_index, p_chosen_neighbour->values().begin() + end_index, value_insertion_it);
            p_chosen_neighbour->values().erase(p_chosen_neighbour->values().begin() + start_index, p_chosen_neighbour->values().begin() + end_index);

            if (is_next)
                adjust_parent_key(p_chosen_neighbour->index());
            else
                adjust_parent_key(p_leaf->index());
        }
        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::delete_node(index_type node_index) {
        std::visit([](auto & node) {
            node.mark_deleted();
        }, node(node_index));
    }

    template<typename Key, typename Value, typename Index, size_t Internal_order, size_t Leaf_order>
    auto btree<Key, Value, Index, Internal_order, Leaf_order>::adjust_parent_key(index_type child_node_index, key_type const *p_correlated_key) -> void {
        if (is_root(child_node_index))
            return;
        if (p_correlated_key == nullptr)
            p_correlated_key = &minimum_key(child_node_index);
        auto const & child_node = node(child_node_index);
        std::visit([this, p_correlated_key](auto & node) {
            assert((node.keys().size() > 0) && "adjust_parent_key(index_type): Child node has no keys!");
            internal_node_type& parent = internal_node(node.parent_index());
            auto [key_it, child_index_it] = parent.iterators_for_index(node.index());
            if (key_it != parent.keys().end())
                *key_it = *p_correlated_key;
            else
                adjust_parent_key(parent.index(), p_correlated_key);
        }, child_node);
    }
} // namespace btree

#endif //BTREE_H

