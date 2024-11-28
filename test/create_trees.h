//
// Created by arnoldm on 20.11.24.
//

#ifndef CREATE_TREES_H
#define CREATE_TREES_H

#include "btree.h"
#include "btree_test_class.h"

namespace bt {

    class btree_test_class;

        void check_equal(auto const & r1, auto const & r2, auto &&proj1 = std::identity{}, auto &&proj2 = std::identity{}) {
        auto const &p1 = std::forward<decltype(proj1)>(proj1);
        auto const &p2 = std::forward<decltype(proj2)>(proj2);
        auto it1 = r1.begin(), last1 = r1.end();
        auto it2 = r2.begin(), last2 = r2.end();
        for (; it1 != last1 && it2 != last2; ++it1, ++it2) {
            CHECK_EQ(p1(*it1), p2(*it2));
        }
        CHECK_EQ(it1, last1);
        CHECK_EQ(it2, last2);
    }

    template<typename T>
    concept range_of_ranges = std::ranges::range<T> &&
        std::ranges::range<std::decay_t<std::invoke_result_t<decltype(std::begin<T>), T>>>;

    struct creators {
        using btree_type = btree_test_class::btree_type;

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

} // namespace bt

#endif //CREATE_TREES_H
