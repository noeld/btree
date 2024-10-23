//
// Created by arnoldm on 23.10.24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "btree.h"

using namespace bt;

TEST_SUITE("test1") {
    TEST_CASE("base operations") {
        btree_internal_node<int, int, int, 5> internal;
        btree_leaf_node<int, int, int, 5> leaf;
        CHECK_EQ(internal.is_leaf(), false);
        CHECK_EQ(leaf.is_leaf(), true);

        CHECK_EQ(internal.size(), 0);
        CHECK_EQ(leaf.size(), 0);
    }
    TEST_CASE("btree") {
        using btree_type = bt::btree<int64_t, double, unsigned int, 64>;
        btree_type tree;

    }
    TEST_CASE("node self") {
        using btree_type = bt::btree<int64_t, double, unsigned int, 64>;
        btree_type::internal_node_type internal;
        CHECK_EQ(internal.keys().size(), 0);
        btree_type::leaf_node_type leaf;
        CHECK_EQ(leaf.keys().size(), 0);
    }
}