//
// Created by arnoldm on 23.10.24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <random>
#include <memory>
#include <ranges>
#include "btree.h"
#include "dyn_array.h"

using namespace bt;

struct TestClass {
    TestClass() {
        ++ctor_counter_;
    }

    explicit TestClass(unsigned value)
        : value_(value) {
        ++ctor_counter_;
    }

    TestClass(const TestClass &other)
        : value_(other.value_) {
        ++ctor_counter_;
    }

    TestClass(TestClass &&other) noexcept
        : value_(other.value_) {
        ++ctor_counter_;
    }

    ~TestClass() {
        --ctor_counter_;
    }

    TestClass & operator=(const TestClass &other) {
        if (this == &other)
            return *this;
        value_ = other.value_;
        return *this;
    }

    TestClass & operator=(TestClass &&other) noexcept {
        if (this == &other)
            return *this;
        value_ = other.value_;
        return *this;
    }

    friend bool operator==(const TestClass &lhs, const TestClass &rhs) {
        return lhs.value_ == rhs.value_;
    }

    friend bool operator!=(const TestClass &lhs, const TestClass &rhs) {
        return !(lhs == rhs);
    }

    friend void swap(TestClass &lhs, TestClass &rhs) noexcept {
        using std::swap;
        swap(lhs.value_, rhs.value_);
    }

    auto operator<=>(const TestClass &other) const {
        return value_ <=> other.value_;
    }

    explicit operator std::string() const { return std::format("TC[{}]", value_); }

    friend std::ostream& operator<<(std::ostream& out, TestClass const & t) {
        return out << t.operator std::string();
    }

    unsigned value_ { value_counter_++ };
    static unsigned value_counter_;
    static unsigned ctor_counter_;
};

unsigned TestClass::ctor_counter_ = 0;
unsigned TestClass::value_counter_ = 0;

struct abstract_action {
    using dyn_array_type = dyn_array<TestClass, 20>;
    using vector_type = std::vector<TestClass>;
    virtual ~abstract_action() = default;
    [[nodiscard]] virtual auto clone() const -> std::unique_ptr<abstract_action> =0;
    virtual void random_init() =0;
    virtual void apply(dyn_array_type&, vector_type&)=0;
    static std::mt19937 rnd_gen_;
};
std::mt19937 abstract_action::rnd_gen_{std::random_device{}()};

struct assign_action : abstract_action {
    [[nodiscard]] auto clone() const -> std::unique_ptr<abstract_action> override {
        return std::make_unique<assign_action>(*this);
    }

    void random_init() override {
        std::uniform_int_distribution<int> distrib{0, (int)dyn_array_.capacity()};
        auto len = distrib(rnd_gen_);
        for(auto i : std::ranges::views::iota(0, len)) {
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

TEST_SUITE("dyn_array") {
    std::tuple<int, double, std::string, TestClass> defaults{0xdeadbeef, 3.1415, "Hallo", TestClass()};
    TEST_CASE_TEMPLATE("Create", T, int, double, std::string, TestClass) {
        unsigned saved_counter = TestClass::ctor_counter_; {
            dyn_array<T, 4> da;
            CHECK(da.empty());
            CHECK_EQ(da.capacity(), 4);
            CHECK_EQ(da.size(), 0);
            CHECK_EQ(std::distance(da.begin(), da.end()), 0);
            CHECK_THROWS_AS(T & t = da.at(0), std::out_of_range);
            for (auto i = 0; i < 4; ++i) {
                da.push_back(std::get<T>(defaults));
                CHECK(!da.empty());
                CHECK_EQ(da.capacity(), 4);
                CHECK_EQ(da.size(), i + 1);
                CHECK_EQ(std::distance(da.begin(), da.end()), i + 1);
                CHECK_NOTHROW(T & t = da.at(i));
                if constexpr (std::is_floating_point_v<T>) {
                    CHECK_EQ(da[i], doctest::Approx(std::get<T>(defaults)));
                    CHECK_EQ(da.front(), doctest::Approx(std::get<T>(defaults)));
                    CHECK_EQ(da.back(), doctest::Approx(std::get<T>(defaults)));
                } else {
                    const auto &res = da[i];
                    CHECK_EQ(res, std::get<T>(defaults));
                    CHECK_EQ(da.front(), std::get<T>(defaults));
                    CHECK_EQ(da.back(), std::get<T>(defaults));
                }
                if constexpr (std::is_same_v<T, TestClass>) {
                    CHECK_EQ(TestClass::ctor_counter_, i + 1 + saved_counter);
                }
            }
        }
        if constexpr (std::is_same_v<T, TestClass>) {
            CHECK_EQ(TestClass::ctor_counter_, saved_counter);
        }
    }

    TEST_CASE("Compare to standard container") {
        std::array<double, 10> test_data{1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1};
        dyn_array<double, 10, uint16_t> arr;
        std::vector<double> vec;
        auto eq = [&arr, &vec] {
            CHECK_EQ(arr.size(), vec.size());
            for (size_t i = 0; i < arr.size(); ++i) {
                CHECK_EQ(arr[i], vec[i]);
            }
        };
        for (auto e: test_data) {
            arr.push_back(e);
            vec.push_back(e);
            eq();
        }
    }
    TEST_CASE("resize") {
        dyn_array<TestClass, 10, uint16_t> arr = { TestClass{5}, TestClass{6}, TestClass{7} };
        arr.resize(1);
        CHECK_EQ(arr.size(), 1);
        CHECK_EQ(arr.at(0), TestClass{5});
        arr.resize(3, TestClass{7});
        CHECK_EQ(arr.size(), 3);
        CHECK_EQ(arr.at(0), TestClass{5});
        CHECK_EQ(arr.at(1), TestClass{7});
        CHECK_EQ(arr.at(2), TestClass{7});
    }
}

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

    TEST_CASE("insert") {
        TestClass tc(0xdeadbeefU);
        using btree_type = bt::btree<TestClass, TestClass, unsigned int, 64>;
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
        using btree_type = btree<int, int, unsigned, 4>;
        btree_type tree;
        for (int i = 1; i < 100; ++i) {
            tree.insert(i, i);
        }
        auto it = tree.begin();
        for (int i = 1; i < 100; ++i) {
            CHECK_EQ((*it).first, i);
            CHECK_EQ((*it).second, i);
            ++it;
        }
    }
        TEST_CASE("random_keys with TestClass") {
        using btree_type = btree<TestClass, std::string, unsigned, 8>;
        btree_type tree;
        auto randomizer = [rnd=std::mt19937{std::random_device{}()},
            dist = std::uniform_int_distribution<unsigned>{1, 10000}]() mutable {
            auto value = dist(rnd);
            return std::make_tuple(TestClass(value), std::format("{}", value));
        };
        for (int i = 0; i < 100; ++i) {
            auto [tc, str] = randomizer();
            tree.insert(tc, str);
        }
    }


}
