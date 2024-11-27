//
// Created by arnoldm on 20.11.24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "btree_test_class.h"
#include "create_trees.h"

using namespace bt;

TEST_SUITE("create_trees") {
    TEST_CASE("create_nlevel_tree") {
        auto tree1 = creators::create_3level_tree(
            {34, 40},
            {{25, 30}, {34, 38}, {47, 50, 52}},
            {{{18, 24}, {25, 26}, {30, 31}}, {{32, 33}, {34, 37}, {39, 40, 41}}, {{42, 45, 46}, {47, 49}, {52, 53}, {54, 58}}}
            );
        using il = std::initializer_list<int>;
        using ill = std::initializer_list<il>;
        auto tree2 = creators::create_nlevel_tree(
                ill{il{1, 2}}
            );
        CHECK(true);
    }
}