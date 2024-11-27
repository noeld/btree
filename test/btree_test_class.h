//
// Created by arnoldm on 17.11.24.
//

#ifndef BTREE_TEST_CLASS_H
#define BTREE_TEST_CLASS_H

#include <doctest/doctest.h>
#include <concepts>
#include <ranges>
#include "btree.h"
#include "dyn_array.h"

namespace bt {


    class btree_test_class {
    public:
        using btree_type = btree<int, int, unsigned, 4, 4>;

        static void check_find_each(btree_type const & tree, auto first, auto last) {
            for (auto it = first; it != last; ++it) {
                auto res = tree.find(*it);
                CHECK_NE(res, tree.end());
                if (res != tree.end())
                    CHECK_EQ(*it, (*res).first);
            }
        }

        static void check_find_each(btree_type const & tree, auto first, auto last, auto&& proj) {
            auto p1 = std::forward<decltype(proj)>(proj);
            for (auto it = first; it != last; ++it) {
                auto res = tree.find(p1(*it));
                CHECK_NE(res, tree.end());
                if (res != tree.end())
                    CHECK_EQ(p1(*it), (*res).first);
            }
        }

        template<typename Btree_type>
        static bool check_sane(Btree_type const & tree, typename Btree_type::internal_node_type const &node) {
            check_sane<Btree_type, typename Btree_type::internal_node_type>(tree, node);
            CHECK_EQ(node.child_indices().size(), node.keys().size() + 1);
            for (btree_type::index_type k = 0; k < node.keys().size(); ++k) {
                auto key = node.keys()[k];
                auto last_key = std::visit([](auto const & child_node) {
                    return child_node.keys().back();
                } , tree.node(node.child_indices()[k]));
                CHECK_LE(last_key, key);
                auto first_key = std::visit([](auto const &child_node) {
                    return child_node.keys().front();
                }, tree.node(node.child_indices()[k + 1]));
                // CHECK_EQ(first_key, key);
                CHECK_GE(first_key, key);
            }
            for(auto index : node.child_indices()) {
                check_sane(tree, tree.node(index));
            }
            return true;
        }
        template<typename Btree_type>
        static bool check_sane(Btree_type const & tree, typename Btree_type::leaf_node_type const &node) {
            check_sane<Btree_type, typename Btree_type::leaf_node_type>(tree, node);
            CHECK_EQ(node.values().size(), node.keys().size());
            if (node.has_previous_leaf_index())
                CHECK_LE(tree.leaf_node(node.previous_leaf_index()).keys().back(), node.keys().front());
            if (node.has_next_leaf_index())
                CHECK_LE(node.keys().back(), tree.leaf_node(node.next_leaf_index()).keys().front());
            if (node.has_parent()) {
                auto const & parent = tree.internal_node(node.parent_index());
                auto [key_it, index_it] = parent.iterators_for_index(node.index());
                CHECK_EQ(*index_it, node.index());
                if (key_it != parent.keys().end())
                    CHECK_LE(*key_it, node.keys().front());
                    // CHECK_EQ(*key_it, node.keys().front());
                CHECK_EQ(*index_it, node.index());
            }
            return true;
        }
        template<typename Btree_type, typename Node_type>
        static bool check_sane(Btree_type const & tree, typename Node_type::base_type const &node) {
            CHECK_LE(node.index(), tree.nodes_.size());
            CHECK_EQ(static_cast<void const *>(&node), static_cast<void const *>(&tree.nodes_[node.index()]));
            if (!tree.is_root(node.index())) {
                CHECK_GE(node.size(), btree_type::traits::get_min_order<std::is_same_v<btree_type::leaf_node_type, typename Node_type::base_type>>());
                CHECK_GE(node.keys().size(), btree_type::traits::get_min_order<std::is_same_v<btree_type::leaf_node_type, typename Node_type::base_type>>());
            }
            CHECK_EQ(node.parent_index() == btree_type::INVALID_INDEX, tree.is_root(node.index()));
            CHECK_EQ(!node.has_parent(), tree.is_root(node.index()));
            CHECK(std::ranges::is_sorted(node.keys()));
            return true;
        }

        template<typename Btree_type>
        static bool check_sane(Btree_type const & tree, typename Btree_type::common_node_type const &node) {
            return std::visit([&tree](auto const & e) {
                return check_sane(tree, e);
            }, node);
        }

        template<typename Btree_type>
        static bool check_sane(Btree_type const & tree) {
            return check_sane(tree, tree.node(tree.root_index()));
        }

    };
}


#endif //BTREE_TEST_CLASS_H
