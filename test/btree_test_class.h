//
// Created by arnoldm on 17.11.24.
//

#ifndef BTREE_TEST_CLASS_H
#define BTREE_TEST_CLASS_H

#include <doctest/doctest.h>
#include "btree.h"
#include "dyn_array.h"

namespace bt {
    template<typename IT1, typename IT2>
    void check_equal(IT1 first1, IT1 last1, IT2 first2, IT2 last2, auto &&proj1 = std::identity{}, auto &&proj2 = std::identity{}) {
        auto const &p1 = std::forward<decltype(proj1)>(proj1);
        auto const &p2 = std::forward<decltype(proj2)>(proj2);
        auto it1 = first1, it2 = first2;
        for (; it1 != last1 && it2 != last2; ++it1, ++it2) {
            CHECK_EQ(p1(*it1), p2(*it2));
        }
        CHECK_EQ(it1, last1);
        CHECK_EQ(it2, last2);
    }

    class btree_test_class {
    public:
        using btree_type = btree<int, int, unsigned, 4, 4>;

        static void check_find_each(btree_type const & tree, auto first, auto last) {
            for (auto it = first; it != last; ++it) {
                auto res = tree.find(*it);
                CHECK_NE(res, tree.end());
                CHECK_EQ(*it, (*res).first);
            }
        }

        template<typename Btree_type>
        static bool check_sane(Btree_type const & tree, typename Btree_type::internal_node_type const &node) {
            check_sane<Btree_type, typename Btree_type::internal_node_type>(tree, node);
            CHECK_EQ(node.child_indices().size(), node.keys().size() + 1);
            for(auto index : node.child_indices())
                check_sane(tree, tree.node(index));
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

        static btree_type create_1level_tree(std::initializer_list<int> root_keys) {
            btree_type tree;
            tree.leaf_node(tree.root_index()).keys() = root_keys;
            tree.leaf_node(tree.root_index()).values() = root_keys;
            return tree;
        }

        static btree_type create_2level_tree(std::initializer_list<int> root_keys, std::vector<std::initializer_list<int>> second_keys) {
            btree_type::internal_node_type root{0};
            root.keys() = root_keys;

            std::vector<btree_type::leaf_node_type> leafes;
            for(btree_type::index_type i = 1; auto e : second_keys) {
                leafes.emplace_back(i, root.index(),  i - 1,  i + 1, e, e);
                root.child_indices().emplace_back(i);
                ++i;
            }
            leafes.front().set_previous_leaf_index(btree_type::INVALID_INDEX);
            leafes.back().set_next_leaf_index(btree_type::INVALID_INDEX);

            btree_type tree;
            tree.nodes_.clear();
            tree.nodes_.emplace_back(root);
            for(auto && e : leafes) {
                tree.nodes_.emplace_back(e);
            }
            return tree;
        }

        static btree_type create_3level_tree(std::initializer_list<int> root_keys,
                                             std::vector<std::initializer_list<int>> second_keys, std::vector<std::vector<std::initializer_list<int>>> third_keys) {
            btree_type tree;
            tree.nodes_.clear();
            btree_type::internal_node_type root{0};
            root.keys() = root_keys;

            btree_type::index_type i = 1;
            std::vector<btree_type::internal_node_type> internals;
            for (auto e: second_keys) {
                internals.emplace_back(i, root.index(), e);
                root.child_indices().emplace_back(i);
                ++i;
            }
            tree.nodes_.emplace_back(root);
            for (auto &&e: internals) {
                tree.nodes_.emplace_back(std::move(e));
            }

            std::vector<btree_type::leaf_node_type> leafes;
            for ( btree_type::index_type parent_i = 1; auto e: third_keys) {
                for (auto ee: e) {
                    leafes.emplace_back(i, parent_i, i - 1, i + 1, ee, ee);
                    // internals[parent_i].child_indices().emplace_back(i);
                    tree.internal_node(parent_i).child_indices().emplace_back(i);
                    ++i;
                }
                ++parent_i;
            }
            leafes.front().set_previous_leaf_index(btree_type::INVALID_INDEX);
            leafes.back().set_next_leaf_index(btree_type::INVALID_INDEX);
            for(auto && e : leafes) {
                tree.nodes_.emplace_back(e);
            }

            return tree;
        }

        static constexpr btree_type test_tree1() {
            return create_3level_tree(
            {34, 40},
            {{25, 30}, {34, 38}, {47, 50, 52}},
            {{{18, 24}, {25, 26}, {30, 31}}, {{32, 33}, {34, 37}, {39, 40, 41}}, {{42, 45, 46}, {47, 49}, {52, 53}, {54, 58}}}
            );
        }

    };
}


#endif //BTREE_TEST_CLASS_H
