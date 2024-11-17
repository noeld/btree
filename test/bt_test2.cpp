//
// Created by arnoldm on 17.11.24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "btree.h"
#include "dyn_array.h"
#include "test_class.h"
#include "btree_test_class.h"

using namespace bt;

TEST_SUITE("btree erase") {
    auto getkey = [](auto const & e) -> decltype(auto) { return e.first; };
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
        check_equal(tree2.begin(), tree2.end(), expected2.begin(), expected2.end(), getkey, std::identity{});
        check_find_each(tree2, expected2.begin(), expected2.end());

        auto expected3 = {1, 2, 3, 4};
        btree_type tree3 = create_3level_tree(
            {5},
            {{3}},
            {{{1, 2}, {3, 4}}}
            );
        tree3.shrink();
        check_sane(tree3);
        check_equal(tree3.begin(), tree3.end(), expected3.begin(), expected3.end(), getkey, std::identity{});
        check_find_each(tree3, expected3.begin(), expected3.end());
    }

    TEST_CASE_FIXTURE(btree_test_class, "rebalance_internal_node") {
        auto expected2 = {1, 2, 3, 4};
        btree_type tree21 = create_2level_tree(
            {5},
            {expected2}
        );
        tree21.rebalance_internal_node(tree21.root_index());
        check_equal(tree21.begin(), tree21.end(), expected2.begin(), expected2.end(), getkey, std::identity{});
        // check_sane(tree21); // tree21 is not sane right from the start

        btree_type tree22 = create_2level_tree(
            {},
            {expected2}
        );
        tree22.rebalance_internal_node(tree22.root_index());
        check_equal(tree22.begin(), tree22.end(), expected2.begin(), expected2.end(), getkey, std::identity{});
        check_find_each(tree22, expected2.begin(), expected2.end());
        check_sane(tree22);

        auto expected3 = {1, 2, 3, 4, 5, 6, 7, 8};
        btree_type tree3 = create_3level_tree(
            {7},
            {{3, 5}, {}},
            {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}} }
            );
        tree3.rebalance_internal_node(2);
        check_sane(tree3);
        check_equal(tree3.begin(), tree3.end(), expected3.begin(), expected3.end(), getkey, std::identity{});
        check_find_each(tree3, expected3.begin(), expected3.end());

        auto expected4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        btree_type tree4 = create_3level_tree(
            {5},
            {{3}, {7, 9}},
            {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}, {9, 10}} }
            );
        tree4.rebalance_internal_node(1);
        std::string tree4_str { tree4};
        check_sane(tree4);
        check_equal(tree4.begin(), tree4.end(), expected4.begin(), expected4.end(), getkey, std::identity{});
        check_find_each(tree4, expected4.begin(), expected4.end());
    }

    TEST_CASE_FIXTURE(btree_test_class, "merge_internal") {

    }
}