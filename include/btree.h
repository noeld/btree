//
// Created by arnoldm on 23.10.24.
//

#ifndef BTREE_H
#define BTREE_H

namespace bt {
    template<typename Key, typename Value, typename Index, size_t Order>
    class btree;

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree_internal_node;

    template<typename Key, typename Value, typename Index, size_t Order>
    class btree_leaf_node;

    template<typename Key, typename Value, typename Index, size_t Order, typename Derived>
    struct btree_node_visitor {
        void visit(this Derived* self, btree_internal_node<Key, Value, Index, Order>& internal_node) {
            self->visit_internal(internal_node);
        }
        void visit(this Derived* self, btree_leaf_node<Key, Value, Index, Order>& leaf_node) {
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
        template<typename Derived_visitor> using visitor_type = btree_node_visitor<Key, Value, Index, Order, Derived_visitor>;

        static constexpr index_type INVALID_INDEX = std::numeric_limits<index_type>::max();
        static constexpr size_t O{Order};

        // Derived* self(this Derived* self) { return self; }
        // Derived const * self(this Derived const* self) { return self; }

        bool is_leaf(this Derived const *self) { return self->is_leaf(); }
        size_t order(this Derived const *self) { return self->O; }

        [[nodiscard]] const index_type &index() const { return index_; }

        size_t size(this Derived const& self) { return self.size_; }

        // auto keys(this Derived const *self) { return std::span(self->keys, self->size_); }
        auto keys(this derived_type& self) { return std::span(self.keys_.data(), self.size_); }

        template<typename Derived_visitor>
        auto accept(this derived_type* self, visitor_type<Derived_visitor>& visitor) -> void {
            visitor.visit(*self);
        }

    protected:
        void set_index(const index_type &index) {
            index_ = index;
        }

    private:
        friend btree_type;
        index_type index_;
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

        static constexpr size_t O{Order}; // The order

        [[nodiscard]] static constexpr bool is_leaf() { return false; }

    protected:

    private:
        friend btree_type;
        friend base_type;
        size_t size_{0};
        std::array<Key, Order> keys_;
        std::array<index_type, Order + 1> child_indices_{INVALID_INDEX};
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

        static constexpr size_t O{Order};

        [[nodiscard]] static constexpr bool is_leaf() { return true; }


        auto values() const { return std::span(values_.data(), size_); }

        decltype(auto) operator[](index_type index) {
            return std::tie(keys_[index], values_ [index]);
        }

    protected:

    private:
        friend btree_type;
        friend base_type;
        index_type previous_leaf_{base_type::INVALID_INDEX};
        index_type next_leaf_{base_type::INVALID_INDEX};
        size_t size_{0};
        std::array<key_type, Order> keys_;
        std::array<value_type, Order> values_;
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

        static constexpr index_type INVALID_INDEX = internal_node_type::INVALID_INDEX;

        static constexpr size_t O{Order};

        auto insert(key_type const &key, value_type const &value) -> bool {
            return true;
        }

        auto contains(key_type const &key) const -> bool {
            return true;
        }

    protected:
    private:
        std::vector<std::variant<internal_node_type, leaf_node_type>> internal_nodes_;
        index_type root_index_{INVALID_INDEX};

    };
} // namespace btree

#endif //BTREE_H
