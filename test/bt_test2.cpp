//
// Created by arnoldm on 17.11.24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#ifndef BTREE_TESTING
#define BTREE_TESTING
#endif
#include "btree.h"
#include "dyn_array.h"
#include "test_class.h"
#include <functional>
#include "btree_test_class.h"

using namespace bt;

// auto getkey(auto const & e) -> decltype(auto) { return (*e).first; };
auto getkey = [](auto const &e)->decltype(auto){return e.first;};

#define TREE_CHECK(tree, expected, action) \
    DOCTEST_SUBCASE((std::string{tree} + " " + #action).c_str()) {\
        CAPTURE(tree);\
        action; \
        check_sane(tree); \
        check_equal(tree, expected, getkey, std::identity{}); \
        check_find_each(tree, expected.begin(), expected.end()); \
        MESSAGE(tree);\
    }


TEST_SUITE("btree erase") {
    TEST_CASE_FIXTURE(btree_test_class, "shrink") {
        btree_type tree1 = create_1level_tree(
            {1, 2}
        );
        check_sane(tree1);
        CHECK_THROWS_AS(tree1.shrink(), std::runtime_error);

        auto expected2 = {1, 2, 3, 4};
        btree_type tree2 = create_2level_tree(
            {5},
            {expected2}
            );
        tree2.shrink();
        check_sane(tree2);
        check_equal(tree2, expected2, getkey, std::identity{});
        check_find_each(tree2, expected2.begin(), expected2.end());

        auto expected3 = {1, 2, 3, 4};
        btree_type tree3 = create_3level_tree(
            {5},
            {{3}},
            {{{1, 2}, {3, 4}}}
            );
        tree3.shrink();
        check_sane(tree3);
        check_equal(tree3, expected3, getkey, std::identity{});
        check_find_each(tree3, expected3.begin(), expected3.end());
    }

    TEST_CASE_FIXTURE(btree_test_class, "rebalance_internal_node") {
        auto expected2 = {1, 2, 3, 4};
        btree_type tree21 = create_2level_tree(
            {5},
            {expected2}
        );
        tree21.rebalance_internal_node(tree21.root_index());
        check_equal(tree21, expected2, getkey, std::identity{});
        // check_sane(tree21); // tree21 is not sane right from the start

        btree_type tree22 = create_2level_tree(
            {},
            {expected2}
        );
        TREE_CHECK(tree22, expected2, (tree22.rebalance_internal_node(tree22.root_index())));

        auto expected3 = {1, 2, 3, 4, 5, 6, 7, 8};
        btree_type tree3 = create_3level_tree(
            {7},
            {{3, 5}, {}},
            {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}} }
            );
        TREE_CHECK(tree3, expected3, (tree3.rebalance_internal_node(2)));


        auto expected4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        btree_type tree4 = create_3level_tree(
            {5},
            {{3}, {7, 9}},
            {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}, {9, 10}} }
            );
        TREE_CHECK(tree4, expected4, (tree4.rebalance_internal_node(1)));
    }

    TEST_CASE_FIXTURE(btree_test_class, "merge_internal") {
        auto expected4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        btree_type tree4 = create_3level_tree(
            {5},
            {{3}, {7, 9}},
            {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}, {9, 10}}}
        );
        TREE_CHECK(tree4, expected4, (tree4.merge_internal(1)));

        auto expected5 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        btree_type tree5 = create_3level_tree(
            {7},
            {{3, 5}, {9}},
            {{{1, 2}, {3, 4}, {5, 6}},{ {7, 8}, {9, 10}}}
        );
        TREE_CHECK(tree5, expected5, (tree5.merge_internal(1)));

        auto expected6 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        btree_type tree6 = create_3level_tree(
            {7, 13},
            {{3, 5}, {9, 11}, {15}},
            {{{1, 2}, {3, 4}, {5, 6}},{ {7, 8}, {9, 10}, {11, 12}}, {{13, 14}, {15, 16}}}
        );
        TREE_CHECK(tree6, expected6, (tree6.merge_internal(2)));

        auto expected7 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        btree_type tree7 = create_3level_tree(
            {7, 11},
            {{3, 5}, {9}, {13, 15}},
            {{{1, 2}, {3, 4}, {5, 6}},{ {7, 8}, {9, 10}}, {{11, 12}, {13, 14}, {15, 16}}}
        );
        TREE_CHECK(tree7, expected7, (tree7.merge_internal(2)));

        auto expected8 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        btree_type tree8 = create_3level_tree(
            {5, 11},
            {{3}, {7, 9}, {13, 15}},
            {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}, {9, 10}}, {{11, 12}, {13, 14}, {15, 16}}}
        );
        TREE_CHECK(tree8, expected8, (tree8.merge_internal(1)));
    }
    TEST_CASE_FIXTURE(btree_test_class, "merge_leaf") {
        auto expected1 = {1, 2, 3, 4, 5};
        auto tree1 = create_2level_tree(
            {3, 5},
            {{1, 2}, {3, 4}, {5}}
            );
        TREE_CHECK(tree1, expected1, tree1.merge_leaf(2));

        auto expected2 = {1, 2, 3, 4, 5};
        auto tree2 = create_2level_tree(
            {3, 4},
            {{1, 2}, {3}, {4, 5}}
            );
        TREE_CHECK(tree2, expected2, tree2.merge_leaf(2));

        auto expected3 = {1, 2, 3, 4, 5};
        auto tree3 = create_2level_tree(
            {2, 4},
            {{1 }, {2, 3}, {4, 5}}
            );
        TREE_CHECK(tree3, expected3, tree3.merge_leaf(1));

        auto expected4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17/*, 18*/};
        btree_type tree4 = create_3level_tree(
            {7, 13},
            {{3, 5}, {9, 11}, {15, 17}},
            {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}, {9, 10}, {11, 12}}, {{13, 14}, {15, 16}, {17/*, 18*/}}}
        );
        TREE_CHECK(tree4, expected4, (tree4.merge_leaf(11)));

        {
            auto expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, /*14,*/ 15, 16, 17, 18};
            btree_type tree = create_3level_tree(
                {7, 13},
                {{3, 5}, {9, 11}, {15, 17}},
                {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}, {9, 10}, {11, 12}}, {{13/*, 14*/}, {15, 16}, {17, 18}}}
            );
            TREE_CHECK(tree, expected, (tree.merge_leaf(10)));
        }
        {
            auto expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /*12,*/ 13, 14, 15, 16, 17, 18};
            btree_type tree = create_3level_tree(
                {7, 13},
                {{3, 5}, {9, 11}, {15, 17}},
                {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}, {9, 10}, {11/*, 12*/}}, {{13, 14}, {15, 16}, {17, 18}}}
            );
            TREE_CHECK(tree, expected, (tree.merge_leaf(9)));
        }
    }
}