//
// Created by arnoldm on 23.10.24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <random>
#include <memory>
#include <ranges>
#include <span>
#include <unordered_set>
#include "btree.h"
#include "dyn_array.h"
#include "test_class.h"

namespace bt {
    class btree_test_class {
    public:
        using btree_type = btree<int, int, unsigned, 4, 4>;

        static void test_merge_leaf() {
            const auto left_keys = { 1};
            const auto right_keys = { 2, 3, 4};
            const auto root_keys = { 2 };

            btree_type::internal_node_type root { 0 };
            root.keys() = root_keys;
            root.child_indices() = { 1, 2};

            btree_type::leaf_node_type left { 1 , 0 };
            left.set_next_leaf_index(2);
            left.keys() = left_keys;
            left.values() = left_keys;

            btree_type::leaf_node_type right { 2, 0 };
            right.set_previous_leaf_index(1);
            right.keys() = right_keys;
            right.values() = right_keys;

            btree_type tree;
            std::vector<btree_type::common_node_type> nodes { root, left, right };
            tree.nodes_ = nodes;

            INFO("btree before is: ", static_cast<std::string>(tree));

            tree.merge_leaf(1);

            INFO("btree after is: ", static_cast<std::string>(tree));

        }
    };
}


using namespace bt;




struct abstract_action {
    using dyn_array_type = dyn_array<TestClass, 20>;
    using vector_type = std::vector<TestClass>;

    virtual ~abstract_action() = default;

    [[nodiscard]] virtual auto clone() const -> std::unique_ptr<abstract_action> =0;

    virtual void random_init() =0;

    virtual void apply(dyn_array_type &, vector_type &) =0;

    static std::mt19937 rnd_gen_;
};

std::mt19937 abstract_action::rnd_gen_{std::random_device{}()};

struct assign_action : abstract_action {
    [[nodiscard]] auto clone() const -> std::unique_ptr<abstract_action> override {
        return std::make_unique<assign_action>(*this);
    }

    void random_init() override {
        std::uniform_int_distribution<int> distrib{0, (int) dyn_array_.capacity()};
        auto len = distrib(rnd_gen_);
        for ([[maybe_unused]] auto i: std::ranges::views::iota(0, len)) {
            auto value = distrib(rnd_gen_);
            dyn_array_.emplace_back(value);
            vector_.emplace_back(value);
        }
    }

    void apply(dyn_array_type &da, vector_type &v) override {
        da = dyn_array_;
        v = vector_;
    }

    abstract_action::dyn_array_type dyn_array_;
    abstract_action::vector_type vector_;
};


TEST_CASE("Compare to standard container with random actions") {
}



TEST_SUITE("btree") {
    TEST_CASE("base operations") {
        using traits = traits_type<int, int, int, 5, 5>;
        btree_internal_node<traits> internal;
        btree_leaf_node<traits> leaf;
        CHECK_EQ(internal.is_leaf(), false);
        CHECK_EQ(leaf.is_leaf(), true);

        CHECK_EQ(internal.size(), 0);
        CHECK_EQ(leaf.size(), 0);
    }

    TEST_CASE("btree") {
        using btree_type = bt::btree<int64_t, double, unsigned int, 64, 64>;
        btree_type tree;
    }

    TEST_CASE("node self") {
        using btree_type = bt::btree<int64_t, double, unsigned int, 64, 64>;
        btree_type::internal_node_type internal;
        CHECK_EQ(internal.keys().size(), 0);
        btree_type::leaf_node_type leaf;
        CHECK_EQ(leaf.keys().size(), 0);
    }

    TEST_CASE("insert") {
        TestClass tc(0xdeadbeefU);
        using btree_type = bt::btree<TestClass, TestClass, unsigned int, 64, 64>;
        btree_type tree;
        CHECK(tree.insert(TestClass(1), tc));
        auto it = tree.begin();
        CHECK_EQ((*it).first, TestClass(1));
        CHECK_EQ((*it).second, TestClass(0xdeadbeefU));
        (*it).second = TestClass(0xabcdef10U);
        CHECK_EQ((*tree.begin()).first, TestClass(1));
        CHECK_EQ((*tree.begin()).second, TestClass(0xabcdef10U));
    }

    TEST_CASE("strictly increasing keys") {
        using btree_type = btree<uint16_t, uint16_t, uint16_t, 4, 4>;
        btree_type tree;
        uint16_t count = 20;
        for (uint16_t i = 1; i < count + 1; ++i) {
            tree.insert(i, i);
        }
        std::string tree_as_string{tree};
        auto it = tree.begin();
        for (uint16_t i = 1; i < count + 1; ++i) {
            CHECK_EQ((*it).first, i);
            CHECK_EQ((*it).second, i);
            ++it;
        }
    }

    TEST_CASE("strictly decreasing keys") {
        using btree_type = btree<uint16_t, uint16_t, uint16_t, 4, 4>;
        btree_type tree;
        uint16_t count = 20;
        for (uint16_t i = count + 1; i > 0; --i) {
            tree.insert(i, i);
        }
        std::string tree_as_string{tree};
        auto it = tree.end();
        for (uint16_t i = count + 1; i > 0; --i) {
            --it;
            CHECK_EQ((*it).first, i);
            CHECK_EQ((*it).second, i);
        }
    }

    TEST_CASE("random_keys with TestClass") {
        using btree_type = btree<TestClass, std::string, unsigned, 8, 8>;
        btree_type tree;
        static constexpr size_t CNT = 100'000;
        std::unordered_set<TestClass> known_keys(100'000);

        unsigned const max_value = 100'000;
        auto randomizer = [rnd=std::mt19937{std::random_device{}()},
                    dist = std::uniform_int_distribution<unsigned>{1, max_value}]() mutable {
            auto value = dist(rnd);
            return std::make_tuple(TestClass(value), std::format("{}", value));
        };
        for (size_t i = 0; i < CNT; ++i) {
            auto [tc, str] = randomizer();
            known_keys.insert(tc);
            tree.insert(tc, str);
        }

        for (auto it = tree.begin(), last_it = it++;
             it != tree.end();
             last_it = it, ++it) {
            CHECK_LE((*last_it).first, (*it).first);
            CHECK_EQ((*it).second, std::format("{}", (*it).first.value_));
        }
        for (auto const &e: known_keys) {
            CHECK(tree.contains(e));
            CHECK_NE(tree.find(e), tree.end());
            CHECK_NE(tree.find_last(e), tree.end());
        }
        CHECK_FALSE(tree.contains(TestClass(max_value + 1)));
        CHECK_FALSE(tree.contains(TestClass(0)));
    }

    TEST_CASE("operator==") {
        using btree_type = btree<int, std::string, unsigned, 8, 8>;
        auto numbers = std::ranges::views::iota(1, 10000);
        std::vector<int> init(numbers.begin(), numbers.end());
        btree_type tree1;
        for (auto const &e: init)
            tree1.insert(e, std::format("{}", e));

        btree_type tree2;
        std::ranges::shuffle(init, std::mt19937{std::random_device{}()});
        for (auto const &e: init) {
            tree2.insert(e, std::format("{}", e));
        }
        CHECK_EQ(tree1, tree2);
    }

    TEST_CASE("find and find_last") {
        using btree_type = btree<int, double, unsigned, 8, 8>;
        btree_type tree;
    }

    TEST_CASE("merge") {
        btree_test_class::test_merge_leaf();
    }
}
