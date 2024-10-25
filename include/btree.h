//
// Created by arnoldm on 23.10.24.
//

#ifndef BTREE_H
#define BTREE_H

#include "dyn_array.h"

namespace bt {
    template<typename Key, typename Value, typename Index, size_t Order>
    class btree;

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

        size_t size(this Derived const &self) { return self.keys_.size(); }

        // auto keys(this Derived const *self) { return std::span(self->keys, self->size_); }
        auto keys(this derived_type &self) { return std::span(self.keys_.data(), self.size()); }

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
        using base_type::size;

        static constexpr size_t O{Order}; // The order

        [[nodiscard]] static constexpr bool is_leaf() { return false; }

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
        using base_type::size;

        static constexpr size_t O{Order};

        [[nodiscard]] static constexpr bool is_leaf() { return true; }


        auto values() const { return std::span(values_.data(), size()); }

        decltype(auto) operator[](index_type index) {
            return std::tie(keys_[index], values_[index]);
        }

    protected:

    private:
        friend btree_type;
        friend base_type;
        index_type previous_leaf_{base_type::INVALID_INDEX};
        index_type next_leaf_{base_type::INVALID_INDEX};
        bt::dyn_array<key_type, Order> keys_;
        bt::dyn_array<value_type, Order> values_;
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
        std::vector<std::variant<internal_node_type, leaf_node_type> > nodes_{leaf_node_type{}};
        index_type root_index_{0};
    };
} // namespace btree

#endif //BTREE_H
