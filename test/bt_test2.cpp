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
#include <random>
#include "btree_test_class.h"

using namespace bt;

// auto getkey(auto const & e) -> decltype(auto) { return (*e).first; };
auto getkey = [](auto const &e)->decltype(auto){return e.first;};

#define TREE_CHECK(name, tree, expected, action) \
    DOCTEST_SUBCASE(name) {\
        auto __tree = tree;\
        CAPTURE(__tree);\
        action; \
        check_sane(__tree); \
        check_equal(__tree, expected, getkey, std::identity{}); \
        check_find_each(__tree, expected.begin(), expected.end()); \
    }


TEST_SUITE("btree") {
    TEST_CASE_FIXTURE(btree_test_class, "shrink") {
        SUBCASE("shrink leaf throws") {
            btree_type tree1 = create_1level_tree(
                {1, 2}
            );
            check_sane(tree1);
            CHECK_THROWS_AS(tree1.shrink(), std::runtime_error);
        }

        auto expected2 = {1, 2, 3, 4};
        TREE_CHECK("shrink 2level", create_2level_tree(
            {5},
            {expected2}
            ), expected2, __tree.shrink());


        auto expected3 = {1, 2, 3, 4};
        TREE_CHECK("shrink 3level", create_3level_tree(
            {5},
            {{3}},
            {{{1, 2}, {3, 4}}}
            ), expected3, __tree.shrink());

    }

    TEST_CASE_FIXTURE(btree_test_class, "rebalance_internal_node") {
        SUBCASE("rebalance_internal_node 2level") {
            auto expected2 = {1, 2, 3, 4};
            btree_type tree21 = create_2level_tree(
                {5},
                {expected2}
            );
            tree21.rebalance_internal_node(tree21.root_index());
            check_equal(tree21, expected2, getkey, std::identity{});
            // check_sane(tree21); // tree21 is not sane right from the start
        }

        auto expected2 = {1, 2, 3, 4};
        TREE_CHECK("empty root", create_2level_tree(
            {},
            {expected2}
        ), expected2, __tree.rebalance_internal_node(__tree.root_index()));

        auto expected3 = {1, 2, 3, 4, 5, 6, 7, 8};
        TREE_CHECK("3level empty right", create_3level_tree(
            {7},
            {{3, 5}, {}},
            {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}} }
            ), expected3, (__tree.rebalance_internal_node(2)));


        auto expected4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        TREE_CHECK("3level left", create_3level_tree(
            {5},
            {{3}, {7, 9}},
            {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}, {9, 10}} }
            ), expected4, (__tree.rebalance_internal_node(1)));
    }

    TEST_CASE_FIXTURE(btree_test_class, "merge_internal") {
        auto expected4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        TREE_CHECK("3level left", create_3level_tree(
            {5},
            {{3}, {7, 9}},
            {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}, {9, 10}}}
        ), expected4, (__tree.merge_internal(1)));

        auto expected5 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        TREE_CHECK("3level right", create_3level_tree(
            {7},
            {{3, 5}, {9}},
            {{{1, 2}, {3, 4}, {5, 6}},{ {7, 8}, {9, 10}}}
        ), expected5, (__tree.merge_internal(1)));

        auto expected6 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        TREE_CHECK("3level 3 middle right", create_3level_tree(
            {7, 13},
            {{3, 5}, {9, 11}, {15}},
            {{{1, 2}, {3, 4}, {5, 6}},{ {7, 8}, {9, 10}, {11, 12}}, {{13, 14}, {15, 16}}}
        ), expected6, (__tree.merge_internal(2)));

        auto expected7 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        TREE_CHECK("3level 3 middle center", create_3level_tree(
            {7, 11},
            {{3, 5}, {9}, {13, 15}},
            {{{1, 2}, {3, 4}, {5, 6}},{ {7, 8}, {9, 10}}, {{11, 12}, {13, 14}, {15, 16}}}
        ), expected7, (__tree.merge_internal(2)));

        auto expected8 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        TREE_CHECK("3level 3 middle left", create_3level_tree(
            {5, 11},
            {{3}, {7, 9}, {13, 15}},
            {{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}, {9, 10}}, {{11, 12}, {13, 14}, {15, 16}}}
        ), expected8, (__tree.merge_internal(1)));
    }
    TEST_CASE_FIXTURE(btree_test_class, "merge_leaf") {
        auto expected1 = {1, 2, 3, 4, 5};
        TREE_CHECK("2level right", create_2level_tree(
            {3, 5},
            {{1, 2}, {3, 4}, {5}}
            ), expected1, __tree.merge_leaf(2));

        auto expected2 = {1, 2, 3, 4, 5};
        TREE_CHECK("2level center",  create_2level_tree(
            {3, 4},
            {{1, 2}, {3}, {4, 5}}
            ), expected2, __tree.merge_leaf(2));

        auto expected3 = {1, 2, 3, 4, 5};
        TREE_CHECK("2level left", create_2level_tree(
            {2, 4},
            {{1 }, {2, 3}, {4, 5}}
            ), expected3, __tree.merge_leaf(1));

        auto expected4 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17/*, 18*/};
        TREE_CHECK("3level right end", create_3level_tree(
            {7, 13},
            {{3, 5}, {9, 11}, {15, 17}},
            {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}, {9, 10}, {11, 12}}, {{13, 14}, {15, 16}, {17/*, 18*/}}}
        ), expected4, (__tree.merge_leaf(11)));

        {
            auto expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, /*14,*/ 15, 16, 17, 18};
            TREE_CHECK("3level left", create_3level_tree(
                {7, 13},
                {{3, 5}, {9, 11}, {15, 17}},
                {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}, {9, 10}, {11, 12}}, {{13/*, 14*/}, {15, 16}, {17, 18}}}
            ), expected, (__tree.merge_leaf(10)));
        }
        {
            auto expected = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /*12,*/ 13, 14, 15, 16, 17, 18};
            TREE_CHECK("3level right middle", create_3level_tree(
                {7, 13},
                {{3, 5}, {9, 11}, {15, 17}},
                {{{1, 2}, {3, 4}, {5, 6}}, {{7, 8}, {9, 10}, {11/*, 12*/}}, {{13, 14}, {15, 16}, {17, 18}}}
            ), expected, (__tree.merge_leaf(9)));
        }
        {
            auto expected = {1, 2, 3, 4, 5, 6, /*7, */8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
            TREE_CHECK("3level right first leaf", create_3level_tree(
                {8, 13},
                {{3, 5}, {9, 11}, {15, 17}},
                {{{1, 2}, {3, 4}, {5, 6}}, {{/*7,*/ 8}, {9, 10}, {11, 12}}, {{13, 14}, {15, 16}, {17, 18}}}
            ), expected, (__tree.merge_leaf(6)));
        }
    }
    TEST_CASE_FIXTURE(btree_test_class, "erase") {
        auto expected1 = {/*18,*/24,25,26,30,31,32,33,34,37,39,40,41,42,45,46,47,49,52,53,54,58};
        TREE_CHECK("erase first (18)", create_3level_tree(
            {32, 42},
            {{25, 30}, {34, 39}, {47, 52, 54}},
            {{{18, 24}, {25, 26}, {30, 31}}, {{32, 33}, {34, 37}, {39, 40, 41}}, {{42, 45, 46}, {47, 49}, {52, 53}, {54, 58}}}
            ), expected1, __tree.erase(__tree.begin()));
        auto expected2 = {348, /*349, */ 350, 351, 352};
        TREE_CHECK("erase second (349)", create_2level_tree(
            {350},
            {{348, 349}, {350, 351, 352}}
            ), expected2, __tree.erase(__tree.find(349)));
        auto expected3 = {431,/*433,*/434,435,439,444,448,450,451,452,455,457,458,460,465,467,468,469};
        TREE_CHECK("erase second (433)", create_3level_tree(
            {450},
            {{434, 439}, {452, 457, 460, 467}},
            {{{431, 433}, {434, 435}, {439, 444, 448}}, {{450, 451}, {452, 455}, {457, 458}, {460, 465}, {467, 468, 469}}}
            ), expected3, __tree.erase(__tree.find(433)));
    }

    TEST_CASE_FIXTURE(btree_test_class, "random insert/erase compare to std::multimap") {
        using map_type = std::multimap<int, int>;

        btree_type tree;
        map_type map;

        auto insert = [&tree, &map](unsigned i) {
            map.insert(std::make_pair(i, i));
            tree.insert(i, i);
        };
        auto erase = [&tree, &map](unsigned i) {
            auto map_it = map.begin();
            std::advance(map_it, i);
            REQUIRE(map_it != map.end());
            auto tree_it = tree.begin();
            for(unsigned j = 0; j < i; ++j)
                ++tree_it;
            REQUIRE(tree_it != tree.end());
            CHECK_EQ(map_it->first, (*tree_it).first);
            CHECK_EQ(map_it->second, (*tree_it).second);
            map.erase(map_it);
            tree.erase(tree_it);
        };
        auto random_nr = [rnd=std::mt19937{std::random_device{}()}](unsigned min, unsigned max) mutable {
            auto dist = std::uniform_int_distribution<unsigned>(min, max);
            return dist(rnd);
        };
        auto random_action = [&random_nr](unsigned ipcnt = 50) {
            static constexpr std::array<char, 3> actions = {'i', 'e'};
            auto nr = random_nr(0, 99);
            int idx = nr < ipcnt;
            return actions[idx];
        };
        btree_test_class::check_sane(tree);
        static constexpr size_t TESTCNT = 1000UL;
        std::string last_action;
        static constexpr std::pair<size_t, unsigned> insert_pcnt[] = {{size_t(0.33*TESTCNT), 75}, {size_t(0.66 * TESTCNT), 50}, {size_t(0.95 * TESTCNT), 25}, {TESTCNT, 0}};
        auto find_pcnt = [&](unsigned i) {
            auto it = std::begin(insert_pcnt);
            while(it->first < i && it != std::end(insert_pcnt)) ++it;
            return it->second;
        };
        unsigned erase_cnt = 0;
        unsigned insert_cnt = 0;
        for (unsigned i = 0; i < 1000; ++i) {
            std::string tree_before = static_cast<std::string>(tree);
            auto ipcnt = find_pcnt(i);
            auto action = random_action(ipcnt);
            if (map.empty())
                action = 'i';
            switch (action) {
                case 'i':
                    ++insert_cnt;
                    last_action = std::format("insert({})", i);
                    insert(i);
                    break;
                case 'e':
                    ++erase_cnt;
                    auto adv = random_nr(0, (unsigned)map.size() - 1);
                    auto it = map.begin();
                    std::advance(it, adv);
                    last_action = std::format("erase({}) -> {}", adv, it->first);
                    erase(adv);
                    break;
            }
            std::string tree_after = static_cast<std::string>(tree);
            check_sane(tree);
            check_equal(tree, map, getkey, [](auto const & e) -> decltype(auto) { return e.first; });
            check_find_each(tree, map.begin(), map.end(), [](auto const & e) -> decltype(auto) { return e.first; });
        }
    }
}