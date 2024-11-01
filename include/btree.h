//
// Created by arnoldm on 23.10.24.
//

#ifndef BTREE_H
#define BTREE_H

#include <functional>

#include "dyn_array.h"

namespace bt {
    template<typename Key, typename Value, typename Index, size_t Order>
    class btree;

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree_iterator;

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree_internal_node;

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree_leaf_node;

    template<typename Key, typename Value, typename Index, size_t Order>
    struct btree_node_visitor {
        void visit(this auto *self, btree_internal_node<Key, Value, Index, Order> &internal_node) {
            self->visit_internal(internal_node);
        }

        void visit(this auto *self, btree_leaf_node<Key, Value, Index, Order> &leaf_node) {
            self->visit_leaf(leaf_node);
        }
    };

    template<typename Key, typename Value, typename Index, size_t Order>
    struct btree_node_select_visitor {
        Index visit(this auto *self, btree_internal_node<Key, Value, Index, Order> &internal_node) {
            self->visit_internal(internal_node);
        }

        Index visit(this auto *self, btree_leaf_node<Key, Value, Index, Order> &leaf_node) {
            self->visit_leaf(leaf_node);
        }
    };

    // template<typename Key, typename Value, typename Index, size_t N>
    // class btree_internal_node;
    //
    // template<typename Key, typename Value, typename Index, size_t N>
    // class btree_leaf_node;

    template<typename Key, typename Value, typename Index, size_t Order, typename Derived>
    class btree_node {
    public:
        using derived_type = Derived;
        using this_type = btree_node;
        using key_type = Key;
        using value_type = Value;
        using index_type = Index;
        using btree_type = btree<Key, Value, Index, Order>;
        using visitor_type = btree_node_visitor<Key, Value, Index, Order>;
        using select_visitor_type = btree_node_select_visitor<Key, Value, Index, Order>;

        static constexpr index_type INVALID_INDEX = std::numeric_limits<index_type>::max();
        static constexpr size_t O{Order};

        // Derived* self(this Derived* self) { return self; }
        // Derived const * self(this Derived const* self) { return self; }

        bool is_leaf(this Derived const *self) { return self->is_leaf(); }
        size_t order(this Derived const *self) { return self->O; }

        [[nodiscard]] const index_type &index() const { return index_; }
        [[nodiscard]] const index_type &parent_index() const { return parent_index_; }

        size_t size(this Derived const &self) { return self.keys_.size(); }

        // auto keys(this Derived const *self) { return std::span(self->keys, self->size_); }
        auto keys(this derived_type &self) -> decltype(auto) { return self.keys(); }

        auto accept(this derived_type *self, visitor_type &visitor) -> void {
            visitor.visit(*self);
        }

        auto accept(this derived_type *self, select_visitor_type &visitor) -> void {
            visitor.visit(*self);
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
        index_type index_;
        index_type parent_index_ = INVALID_INDEX;
    };

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree_internal_node : public btree_node<Key, Value, Index, Order, btree_internal_node<Key, Value, Index,
                Order> > {
    public:
        using key_type = Key;
        using value_type = Value;
        using index_type = Index;
        using this_type = btree_internal_node;
        using base_type = btree_node<Key, Value, Index, Order, this_type>;
        using btree_type = btree<Key, Value, Index, Order>;

        using base_type::INVALID_INDEX;
        using base_type::index;
        using base_type::set_index;
        using base_type::parent_index;
        using base_type::set_parent_index;
        using base_type::size;
        using base_type::accept;

        static constexpr size_t O{Order}; // The order

        [[nodiscard]] static constexpr bool is_leaf() { return false; }

        [[nodiscard]] bt::dyn_array<Key, Order>& keys() { return keys_; }

        [[nodiscard]] dyn_array<index_type, Order + 1>& child_indices() { return child_indices_; }

    protected:

    private:
        friend btree_type;
        friend base_type;
        bt::dyn_array<Key, Order> keys_;
        bt::dyn_array<index_type, Order + 1> child_indices_{INVALID_INDEX};
    };

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree_leaf_node : public btree_node<Key, Value, Index, Order, btree_leaf_node<Key, Value, Index, Order> > {
    public:
        using key_type = Key;
        using value_type = Value;
        using index_type = Index;
        using this_type = btree_leaf_node;
        using base_type = btree_node<Key, Value, Index, Order, this_type>;
        using btree_type = btree<Key, Value, Index, Order>;

        using base_type::INVALID_INDEX;
        using base_type::index;
        using base_type::set_index;
        using base_type::parent_index;
        using base_type::set_parent_index;
        using base_type::size;
        using base_type::accept;

        static constexpr size_t O{Order};

        [[nodiscard]] static constexpr bool is_leaf() { return true; }

        [[nodiscard]] bt::dyn_array<Key, Order>& keys() { return keys_; }

        [[nodiscard]] bt::dyn_array<Value, Order>&  values() { return values_; }

        decltype(auto) operator[](index_type index) {
            return std::tie(keys_[index], values_[index]);
        }

        [[nodiscard]] ::bt::btree_leaf_node<Key, Value, Index, Order>::index_type previous_leaf_index() const {
            return previous_leaf_index_;
        }

        [[nodiscard]] auto has_previous_leaf_index() const { return previous_leaf_index() != INVALID_INDEX; }

        void set_previous_leaf_index(
            const ::bt::btree_leaf_node<Key, Value, Index, Order>::index_type &previous_leaf_index) {
            previous_leaf_index_ = previous_leaf_index;
        }

        [[nodiscard]] ::bt::btree_leaf_node<Key, Value, Index, Order>::index_type next_leaf_index() const {
            return next_leaf_index_;
        }

        [[nodiscard]] auto has_next_leaf_index() const { return next_leaf_index() != INVALID_INDEX; }

        void set_next_leaf_index(const ::bt::btree_leaf_node<Key, Value, Index, Order>::index_type &next_leaf_index) {
            next_leaf_index_ = next_leaf_index;
        }

    protected:

    private:
        friend btree_type;
        friend base_type;
        index_type previous_leaf_index_{base_type::INVALID_INDEX};


    private:
        index_type next_leaf_index_{base_type::INVALID_INDEX};
        bt::dyn_array<key_type, Order> keys_;
        bt::dyn_array<value_type, Order> values_;
    };

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree_iterator {
    public:
        using key_type = Key;
        using value_type = Value;
        using index_type = Index;
        using this_type = btree_iterator;
        using btree_type = btree<Key, Value, Index, Order>;
        using leaf_node_type = typename btree_type::leaf_node_type;
        using internal_node_type = typename btree_type::internal_node_type;
        using common_node_type = typename btree_type::common_node_type;

        static constexpr index_type INVALID_INDEX = internal_node_type::INVALID_INDEX;

        btree_iterator(btree_type &btree, const index_type &leaf_node_index = INVALID_INDEX, const index_type &leaf_index = index_type(0))
            : btree_(btree), leaf_node_index_(leaf_node_index), leaf_index_(leaf_index) {
        }

        btree_iterator(const btree_iterator &other)
            : btree_(other.btree_),
              leaf_node_index_(other.leaf_node_index_),
              leaf_index_(other.leaf_index_) {
        }

        btree_iterator & operator=(const btree_iterator &other) {
            if (this == &other)
                return *this;
            btree_ = other.btree_;
            leaf_node_index_ = other.leaf_node_index_;
            leaf_index_ = other.leaf_index_;
            return *this;
        }

        friend bool operator==(const btree_iterator &lhs, const btree_iterator &rhs) {
            return std::tie(&lhs.btree_, lhs.leaf_node_index_, lhs.leaf_index_)
                == std::tie(&rhs.btree_, rhs.leaf_node_index_, rhs.leaf_index_);
        }

        btree_iterator & operator++() {
            auto& node = current_leaf();
            ++leaf_index_;
            if (leaf_index_ >= node.keys().size())
                go_forward();
            return *this;
        }

        btree_iterator operator++(int) {
            btree_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        btree_iterator & operator--() {
            auto& node = current_leaf();
            if (leaf_index_ == 0)
                go_backward();
            else
                --leaf_index_;
            return *this;
        }

        btree_iterator operator--(int) {
            btree_iterator tmp = *this;
            --*this;
            return tmp;
        }

        auto operator*() {
            auto& node = current_leaf();
            assert(("key index out of bounds", leaf_index_ < node.keys().size()));
            assert(("value index out of bounds", leaf_index_ < node.values().size()));
            return std::make_pair(
                std::ref(node.keys()[leaf_index_]), std::ref(node.values()[leaf_index_]));
        };

        // auto operator->() -> std::pair<key_type const&, value_type &> {
        //     return this->operator*();
        // }

    protected:
        auto current_leaf() -> leaf_node_type& {
            return btree_.leaf_node(leaf_node_index_);
        }

        auto go_forward() {
            auto& node = current_leaf();
            if (node.has_next_leaf_index()) {
                leaf_node_index_ = node.next_leaf_index();
                leaf_index_ = 0;
            } else {
                set_end();
            }
        }
        auto go_backward() {
            auto& node = current_leaf();
            if (node.has_previous_leaf_index()) {
                leaf_node_index_ = node.previous_leaf_index();
                leaf_index_ = node.keys().size() > 0 ? node.keys().size() - 1 : 0;
            } else {
                set_end();
            }
        }
        auto set_end() {
            leaf_node_index_ = INVALID_INDEX;
            leaf_index_ = 0;
        }
    private:
        friend btree_type;

        btree_type& btree_;
        index_type leaf_node_index_ = INVALID_INDEX;
        index_type leaf_index_ = 0;
    };

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree {
    public:
        using key_type = Key;
        using value_type = Value;
        using index_type = Index;
        using this_type = btree;
        using internal_node_type = btree_internal_node<Key, Value, Index, Order>;
        using leaf_node_type = btree_leaf_node<Key, Value, Index, Order>;
        using common_node_type = std::variant<internal_node_type, leaf_node_type>;

        using iterator = btree_iterator<Key, Value, Index, Order>;
        using const_iterator = const iterator;

        static constexpr index_type INVALID_INDEX = internal_node_type::INVALID_INDEX;

        static constexpr size_t O{Order};

        auto begin() -> iterator;
        auto begin() const -> const_iterator { return cbegin(); }
        auto cbegin() const -> const_iterator;

        auto end() -> iterator { return iterator(*this); }
        auto end() const -> iterator { return iterator(*this); }
        auto cend() const -> const_iterator { return const_iterator(*this); };

        auto insert(key_type const &key, value_type const &value) -> bool ;

        auto erase(key_type const& key) -> bool;

        auto contains(key_type const &key) const -> bool { return true; }

        auto get(const key_type &key) const -> const value_type&;

        auto get_or(const key_type &key, const value_type &default_value = value_type()) -> value_type&;

    protected:
        auto is_root(index_type index) const noexcept -> bool {
            return index == root_index_;
        }
        auto is_root(leaf_node_type const & node) const noexcept -> bool {
            return is_root(node.index());
        }
        auto is_root(internal_node_type const & node) const noexcept -> bool {
            return is_root(node.index());
        }
        auto node(index_type const & index) -> common_node_type& {
            assert(("node index out of bounds", index < nodes_.size()));
            if (index < nodes_.size())
                throw std::out_of_range("node index out of bounds");
            return nodes_[index];
        }

        auto leaf_node(index_type const & index) -> leaf_node_type& {
            common_node_type& r_node = node(index);
            assert(("index is not a leaf node", std::holds_alternative<leaf_node_type>(r_node)));
            leaf_node_type* p_leaf = std::get_if<leaf_node_type>(&r_node);
            if (p_leaf == nullptr)
                throw std::runtime_error("index does no denote a leaf node");
            return *p_leaf;
        }

        auto internal_node(index_type const & index) -> internal_node_type& {
            common_node_type& r_node = node(index);
            assert(("index is not a leaf node", std::holds_alternative<leaf_node_type>(r_node)));
            internal_node_type* p_internal = std::get_if<internal_node_type>(&r_node);
            if (p_internal == nullptr)
                throw std::runtime_error("index does no denote a internal node");
            return *p_internal;
        }

        auto create_internal_node(index_type const &parent_index = INVALID_INDEX) -> index_type;

        auto create_leaf_node(index_type const &parent_index = INVALID_INDEX) -> index_type;

        auto find_insert_position(const key_type &key) -> iterator;

        auto insert_split_internal(index_type node_index, const key_type &key, index_type child_index) -> bool;

        auto insert_internal(index_type node_index, const key_type &key, index_type child_index) -> bool;

        auto insert_split_leaf(iterator insert_pos, const key_type &key, const value_type &value) -> bool;


    private:
        friend class btree_iterator<Key, Value, Index, Order>;

        std::vector<common_node_type> nodes_{leaf_node_type{}};
        index_type root_index_{0};
    };

    template<typename Key, typename Value, typename Index, size_t Order>
    auto btree<Key, Value, Index, Order>::begin() -> iterator {
        common_node_type* p_node = nullptr;
        index_type next_node_index = root_index_;
        do {
            if (next_node_index == INVALID_INDEX) {
                assert(("should never happen", false));
                return end();
            }
            p_node = &node(next_node_index);
            if (std::holds_alternative<internal_node_type>(*p_node)) {
                internal_node_type& internal_node = std::get<internal_node_type>(*p_node);
                assert(("internal_node_type must have at least one leaf child", internal_node.child_indices().size() > 0));
                next_node_index = internal_node.child_indices()[0];
            }
        } while (std::holds_alternative<internal_node_type>(*p_node));
        return iterator(*this, next_node_index);
    }

    template<typename Key, typename Value, typename Index, size_t Order>
    auto btree<Key, Value, Index, Order>::insert(key_type const &key, value_type const &value) -> bool {
        iterator it = find_insert_position(key);
        leaf_node_type& leaf = it.current_leaf();
        if (leaf.size() < leaf.O) {
            leaf.keys_.insert(leaf.keys_.begin() + it.leaf_index_, key);
            leaf.values_.insert(leaf.values_.begin() + it.leaf_index_, value);
        } else {
            insert_split_leaf(it, key, value);
        }
        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Order>
    auto btree<Key, Value, Index, Order>::create_internal_node(index_type const &parent_index) -> index_type {
        auto index = nodes_.size();
        internal_node_type new_internal;
        new_internal.index_ = index;
        new_internal.parent_index_ = parent_index;
        nodes_.emplace_back(std::move(new_internal));
        return index;
    }

    template<typename Key, typename Value, typename Index, size_t Order>
    auto btree<Key, Value, Index, Order>::create_leaf_node(index_type const &parent_index) -> index_type {
        auto index = nodes_.size();
        leaf_node_type new_leaf;
        new_leaf.index_ = index;
        new_leaf.parent_index_ = parent_index;
        nodes_.emplace_back(std::move(new_leaf));
        return index;
    }

    template<typename Key, typename Value, typename Index, size_t Order>
    auto btree<Key, Value, Index, Order>::find_insert_position(const key_type &key) -> iterator {
        index_type node_index = root_index_;
        do {
            if (node_index == INVALID_INDEX) {
                assert(("should never happen", false));
                return end();
            }
            common_node_type *p_node = &node(node_index);
            auto result = std::visit([&](auto &node) -> std::variant<index_type, iterator> { // TODO: lambda must return same type for each branch!
                auto found = std::ranges::upper_bound(node.keys(), key); // found > key
                index_type found_index = std::distance(node.keys().begin(), found);
                if constexpr (std::is_same_v<std::decay_t<decltype(node)>, internal_node_type>) {
                    auto previous_index =  found_index > 0 ? found_index - 1 : 0; // previous <= key
                    node_index = node.child_indices_[previous_index];
                    return node_index;
                } else {
                    static_assert(std::is_same_v<std::decay_t<decltype(node)>, leaf_node_type>);
                    return iterator(*this, node_index, found_index);
                }
            }, *p_node);
            if (std::holds_alternative<iterator>(result))
                return std::get<iterator>(result);
        } while (true);
        return end();
    }

    template<typename Key, typename Value, typename Index, size_t Order>
    auto btree<Key, Value, Index, Order>::insert_split_internal(index_type node_index, const key_type &key,
        index_type child_index) -> bool {
        assert(("internal node should be full", internal_node(node_index).size() == internal_node_type::O));

        // create new internal
        index_type new_internal_index = create_internal_node(internal_node(node_index).parent_index());
        internal_node_type& new_internal = internal_node(new_internal_index);

        internal_node_type* p_internal = &internal_node(node_index);

        auto pivot_key_it = std::midpoint(p_internal->keys().begin(), p_internal->keys().end());
        auto pivot_index = std::distance(p_internal->keys().begin(), pivot_key_it);
        auto pivot_child_indices_it = p_internal->child_indices().begin() + pivot_index;

        // save pivot
        key_type pivot_key = *pivot_key_it;

        // move pivot keys/child_indices and everything right into new node
        std::move(pivot_key_it, p_internal->keys().end(), std::back_inserter(new_internal.keys()));
        std::move(pivot_child_indices_it, p_internal->child_indices().end(), std::back_inserter(new_internal.child_indices()));

        // shrink left node
        p_internal->keys().resize(pivot_index - 1);
        p_internal->child_indices().resize(pivot_index - 1);

        // insert key and value into one of the leaf nodes
        internal_node_type* p_insert_internal = (key < pivot_key) ? p_internal : &new_internal;
        auto insert_key_pos_it = std::upper_bound(p_insert_internal->keys().begin(), p_insert_internal->keys().end(), key);
        auto insert_values_it = p_insert_internal->child_indices().begin() + std::distance(p_insert_internal->keys().begin(), insert_key_pos_it);
        p_insert_internal->keys().insert(insert_key_pos_it, key);
        p_insert_internal->child_indices().insert(insert_values_it, child_index);

        if (is_root(*p_internal)) {
            // TODO
        } else {
            insert_internal(p_internal->parent_index(), pivot_key, new_internal_index);
        }

        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Order>
    auto btree<Key, Value, Index, Order>::insert_internal(index_type node_index, const key_type &key,
        index_type child_index) -> bool {
        internal_node_type& node = internal_node(node_index);
        if (node.size() < node.O) {
            auto insert_pos_it = std::upper_bound(node.keys().begin(), node.keys().end(), key);
            auto insert_pos_index = std::distance(node.keys().begin(), insert_pos_it);
            node.keys().insert(insert_pos_it, key);
            node.child_indices().insert(node.child_indices().begin() + insert_pos_index, child_index);
        } else {
            insert_split_internal(node_index, key, child_index);
        }
        return true;
    }

    template<typename Key, typename Value, typename Index, size_t Order>
    auto btree<Key, Value, Index, Order>::insert_split_leaf(iterator insert_pos, const key_type &key, const value_type &value)-> bool {
        assert(("leaf node should be full", insert_pos.current_leaf().keys().size() == insert_pos.current_leaf().keys().capacity()));

        // create a new leaf
        index_type new_leaf_index = create_leaf_node(insert_pos.current_leaf().parent_index());
        leaf_node_type& new_leaf = leaf_node(new_leaf_index);

        // p_leaf
        leaf_node_type* p_leaf = &insert_pos.current_leaf();

        auto pivot_key_it = std::midpoint(p_leaf->keys().begin(), p_leaf->keys().end());
        auto pivot_value_it = std::midpoint(p_leaf->values().begin(), p_leaf->values().end());
        auto pivot_index = std::distance(p_leaf->keys().begin(), pivot_key_it);

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
            auto next_leaf = leaf_node(new_leaf.next_leaf_index());
            next_leaf.set_previous_leaf_index(new_leaf.index());
        }

        // shrink left node
        p_leaf->keys().resize(pivot_index - 1);
        p_leaf->values().resize(pivot_index - 1);

        // insert key and value into one of the leaf nodes
        leaf_node_type* p_insert_leaf = (key < pivot_key) ? p_leaf : &new_leaf;
        auto insert_key_pos_it = std::upper_bound(p_insert_leaf->keys().begin(), p_insert_leaf->keys().end(), key);
        auto insert_values_it = p_insert_leaf->values().begin() + std::distance(p_insert_leaf->keys().begin(), insert_key_pos_it);
        p_insert_leaf->keys().insert(insert_key_pos_it, key);
        p_insert_leaf->values().insert(insert_values_it, value);

        if (is_root(*p_leaf)) {
            // TODO
        } else {
            // insert pivot_key into parent (internal) node
            insert_internal(p_leaf->parent_index(), pivot_key, new_leaf_index);
        }

        return true;
    }
} // namespace btree

#endif //BTREE_H
