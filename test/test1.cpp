//
// Created by arnoldm on 23.10.24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
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

TEST_SUITE("dyn_array") {
    std::tuple<int, double, std::string, TestClass> defaults{0xdeadbeef, 3.1415, "Hallo", TestClass()};
    TEST_CASE_TEMPLATE("Create", T, int, double, std::string, TestClass) {
        unsigned saved_counter = TestClass::ctor_counter_;
        {
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
                    const auto& res = da[i];
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
        std::array<double, 10> test_data { 1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1};
        dyn_array<double, 10, uint16_t> arr;
        std::vector<double> vec;
        auto eq = [&arr, &vec] {
            CHECK_EQ(arr.size(), vec.size());
            for (size_t i = 0; i < arr.size(); ++i) {
                CHECK_EQ(arr[i], vec[i]);
            }
        };
        for(auto e : test_data) {
            arr.push_back(e);
            vec.push_back(e);
            eq();
        }
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
        TestClass tc;
        using btree_type = bt::btree<int64_t, TestClass, unsigned int, 64>;
        btree_type tree;
        tree.insert(1, tc);
    }
}
