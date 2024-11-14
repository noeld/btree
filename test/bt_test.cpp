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
    template<typename IT1, typename IT2>
    bool equal_sequence(IT1 first1, IT1 last1, IT2 first2, IT2 last2, auto &&proj1, auto &&proj2) {
        auto const &p1 = std::forward<decltype(proj1)>(proj1);
        auto const &p2 = std::forward<decltype(proj2)>(proj2);
        auto it1 = first1, it2 = first2;
        for (; it1 != last1 && it2 != last2; ++it1, ++it2) {
            if (p1(*it1) != p2(*it2))
                return false;
        }
        return it1 == last1 && it2 == last2;
    }

    class btree_test_class {
    public:
        using btree_type = btree<int, int, unsigned, 4, 4>;

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
                tree.nodes_.emplace_back(e);
            }

            std::vector<btree_type::leaf_node_type> leafes;
            for ( btree_type::index_type parent_i = 1; auto e: third_keys) {
                for (auto ee: e) {
                    leafes.emplace_back(i, parent_i, i - 1, i + 1, ee, ee);
                    internals[parent_i].child_indices().emplace_back(i);
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

        static void test_merge_leaf() {
            const auto left_keys = {1};
            const auto right_keys = {2, 3};
            const auto root_keys = {2};
            const auto all_keys = {1, 2, 3};

            btree_type::internal_node_type root{0};
            root.keys() = root_keys;
            root.child_indices() = {1, 2};

            btree_type::leaf_node_type left{1, 0};
            left.set_next_leaf_index(2);
            left.keys() = left_keys;
            left.values() = left_keys;

            btree_type::leaf_node_type right{2, 0};
            right.set_previous_leaf_index(1);
            right.keys() = right_keys;
            right.values() = right_keys;

            btree_type tree;
            std::vector<btree_type::common_node_type> nodes{root, left, right};
            tree.nodes_ = nodes;

            INFO("btree before is: ", static_cast<std::string>(tree));

            tree.merge_leaf(1);

            INFO("btree after is: ", static_cast<std::string>(tree));
            auto res = equal_sequence(tree.begin(), tree.end(), all_keys.begin(), all_keys.end(),
                                      [](auto const &e) -> decltype(auto) { return e.first; }, std::identity{});
            CHECK(res);
            check_sane(tree);
        }

        static void test_rebalance_leaf() {
            const auto left_keys = {1};
            const auto right_keys = {2, 3, 4};
            const auto root_keys = {2};
            const auto all_keys = {1, 2, 3, 4};

            btree_type::internal_node_type root{0};
            root.keys() = root_keys;
            root.child_indices() = {1, 2};

            btree_type::leaf_node_type left{1, 0};
            left.set_next_leaf_index(2);
            left.keys() = left_keys;
            left.values() = left_keys;

            btree_type::leaf_node_type right{2, 0};
            right.set_previous_leaf_index(1);
            right.keys() = right_keys;
            right.values() = right_keys;

            btree_type tree;
            std::vector<btree_type::common_node_type> nodes{root, left, right};
            tree.nodes_ = nodes;

            INFO("btree before is: ", static_cast<std::string>(tree));
            tree.rebalance_leaf_node(1);
            INFO("btree after is: ", static_cast<std::string>(tree));
            auto res = equal_sequence(tree.begin(), tree.end(), all_keys.begin(), all_keys.end(),
                                      [](auto const &e) -> decltype(auto) { return e.first; }, std::identity{});
            CHECK(res);
            check_sane(tree);
        }

        static void test_erase() {
            const auto left_keys = {1, 2, 3};
            const auto right_keys = {4, 5, 6};
            const auto root_keys = {4};
            std::set<int> ref(left_keys);
            ref.insert(right_keys);

            btree_type::internal_node_type root{0};
            root.keys() = root_keys;
            root.child_indices() = {1, 2};

            btree_type::leaf_node_type left{1, 0};
            left.set_next_leaf_index(2);
            left.keys() = left_keys;
            left.values() = left_keys;

            btree_type::leaf_node_type right{2, 0};
            right.set_previous_leaf_index(1);
            right.keys() = right_keys;
            right.values() = right_keys;

            btree_type tree;
            std::vector<btree_type::common_node_type> nodes{root, left, right};
            tree.nodes_ = nodes;
            check_sane(tree);

            for (auto e: {5, 4, 2, 3, 1, 6}) {
                INFO("btree before is: ", static_cast<std::string>(tree));
                auto it = tree.find(e);
                CHECK_NE(it, tree.end());
                CHECK_EQ((*it).first, e);
                CHECK_EQ((*it).second, e);
                tree.erase(it);
                ref.erase(e);
                CHECK(equal_sequence(tree.begin(), tree.end(), ref.begin(), ref.end(), [](auto const & e) -> decltype(
                    auto) { return e.first; }, std::identity{}));
                check_sane(tree);
            }
        }
    };
};



using namespace bt;

struct serializable {
    virtual ~serializable() = default;



};

struct action_base {
    using btree_type = btree<TestClass, std::string, unsigned, 4, 4>;
    using map_type = std::multimap<TestClass, std::string>;

    action_base() = default;

    action_base(unsigned id, unsigned i)
        : id_(id),
          i_(i) {
    }

    virtual ~action_base() = default;

    friend auto operator<=>(const action_base &lhs, const action_base &rhs) {
        return lhs.id_ <=> rhs.id_;
    }

    friend bool operator==(const action_base &lhs, const action_base &rhs) {
        return lhs.id_ == rhs.id_
               && lhs.i_ == rhs.i_;
    }

    friend bool operator!=(const action_base &lhs, const action_base &rhs) {
        return !(lhs == rhs);
    }

    virtual void apply(btree_type& tree) const = 0;
    virtual void apply(map_type& map) const = 0;

    constexpr TestClass test_class() const noexcept { return TestClass(i_); }
    constexpr std::string string() const noexcept { return std::format("{}", i_); }

protected:
    static std::atomic_uint ID;
    unsigned id_ { ++ID };
    unsigned i_ {0};
};
std::atomic_uint action_base::ID { 0 };


struct insert_action : public action_base {
    void apply(btree_type &tree) const override {
        auto str = string();
        auto tc = test_class();
        tree.insert(tc, str);
    };

    void apply(map_type &map) const override {
        auto str = string();
        auto tc = test_class();
        map.insert(std::make_pair(tc, str));
    };
};

struct erase_action : public action_base {
    void apply(btree_type &tree) const override {
        auto tree_it = tree.begin();
        for (unsigned j = 0; j < i_; ++j)
            ++tree_it;
        REQUIRE(tree_it != tree.end());
        tree.erase(tree_it);
    };

    void apply(map_type &map) const override {
        auto map_it = map.begin();
        std::advance(map_it, i_);
        REQUIRE(map_it != map.end());
        map.erase(map_it);
    };
};

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

    TEST_CASE("test_merge_leaf") {
        btree_test_class::test_merge_leaf();
    }

    TEST_CASE("test_rebalance_leaf") {
        btree_test_class::test_rebalance_leaf();
    }

    TEST_CASE("test_erase") {
        btree_test_class::test_erase();
    }

    TEST_CASE_FIXTURE(btree_test_class, "erase1") {
        auto tree = test_tree1();
        check_sane(tree);
        std::string tree_str{tree};
        tree.erase(tree.begin());
        check_sane(tree);
    }

    TEST_CASE("random insert/erase compare to std::multimap") {
        using btree_type = btree<TestClass, std::string, unsigned, 4, 4>;
        using map_type = std::multimap<TestClass, std::string>;

        btree_type tree;
        map_type map;

        auto insert = [&tree, &map](unsigned i) {
            auto str = std::format("{}", i);
            auto tc = TestClass(i);
            map.insert(std::make_pair(tc, str));
            tree.insert(tc, str);
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
        auto random_action = [&random_nr]() {
            static constexpr char actions[] = {'i', 'e'};
            return actions[random_nr(0, 1)];
        };
        btree_test_class::check_sane(tree);
        std::string last_action;
        for (unsigned i = 0; i < 1000; ++i) {
            std::string tree_before = static_cast<std::string>(tree);
            auto action = random_action();
            if (map.empty())
                action = 'i';
            switch (action) {
                case 'i':
                    last_action = std::format("insert({})", i);
                    insert(i);
                    break;
                case 'e':
                    auto adv = random_nr(0, (unsigned)map.size() - 1);
                    auto it = map.begin();
                    std::advance(it, adv);
                    last_action = std::format("erase({}) -> {}", adv, (*it).first.value_);
                    erase(adv);
                    break;
            }
            std::string tree_after = static_cast<std::string>(tree);
            CHECK(equal_sequence(tree.begin(), tree.end(), map.begin(), map.end(),
                [](auto const &a) -> decltype(auto) { return a.first; },
                [](auto const &b) -> decltype(auto) { return b.first; }));
            btree_test_class::check_sane(tree);
        }
    }
}
