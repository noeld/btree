//
// Created by arnoldm on 11.11.24.
//
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <random>
#include <memory>
#include <ranges>
#include <span>
#include "test_class.h"
#include "dyn_array.h"

TEST_SUITE("dyn_array") {
    using namespace bt;
    std::tuple<int, double, std::string, TestClass> defaults{0xdeadbeef, 3.1415, "Hallo", TestClass()};
    TEST_CASE_TEMPLATE("Create", T, int, double, std::string, TestClass) {
        unsigned saved_counter = TestClass::ctor_counter_; {
            dyn_array<T, 4> da;
            CHECK(da.empty());
            CHECK_EQ(da.capacity(), 4);
            CHECK_EQ(da.size(), 0);
            CHECK_EQ(std::distance(da.begin(), da.end()), 0);
            CHECK_THROWS_AS([[maybe_unused]] auto ignore = da.at(0), std::out_of_range);
            for (auto i = 0; i < 4; ++i) {
                da.push_back(std::get<T>(defaults));
                CHECK(!da.empty());
                CHECK_EQ(da.capacity(), 4);
                CHECK_EQ(da.size(), i + 1);
                CHECK_EQ(std::distance(da.begin(), da.end()), i + 1);
                CHECK_NOTHROW([[maybe_unused]] auto ignore = da.at(i));
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
    TEST_CASE("insert_space") {
            auto my_init = {1, 2, 3, 4, 5};
            using arr_type = dyn_array<int, 10, uint16_t>;
            auto cmp = [&my_init](arr_type &arr, arr_type::size_type pos, arr_type::size_type n) {
                CHECK(std::ranges::equal(std::span(arr.begin(), arr.begin() + pos), std::span(my_init.begin(), my_init.begin() + pos)));
                CHECK(std::ranges::equal(std::span(arr.begin() + pos + n, arr.end()), std::span(my_init.begin() + pos, my_init.end())));
            };
            for(auto n : {arr_type::size_type{0}, arr_type::size_type{2}}) {
                {
                    arr_type arr(my_init);
                    REQUIRE(arr.size() == my_init.size());
                    arr.insert_space(arr.begin(), n);
                    cmp(arr, 0, n);
                } {
                    arr_type arr(my_init);
                    REQUIRE(arr.size() == my_init.size());
                    arr.insert_space(arr.begin() + 2, n);
                    cmp(arr, 2, n);
                } {
                    arr_type arr(my_init);
                    REQUIRE(arr.size() == my_init.size());
                    arr.insert_space(arr.end() - 1, n);
                    cmp(arr, arr.size() - 2, n);
                }
            }

            // {
            //     arr_type arr(init);
            //     arr.insert_space(arr.begin(), 2);
            //     CHECK_EQ(arr.size(), 5 + 2);
            //
            // }
            //
            // arr.insert_space(arr.begin() + 2, 2);
            // CHECK_EQ(arr.size(), 5 + 2 + 2);
            // CHECK(std::ranges::equal(std::span(arr.begin() + 3, arr.end()), std::span(init.begin(), init.end())));
            // arr.insert_space(arr.end() - 1, 2);
            // CHECK(std::ranges::equal(std::span(arr.begin() + 3, arr.end()), std::span(init.begin(), init.end())));
            //
            // arr.insert_space(arr.begin(), 3);
            // CHECK_EQ(arr.size(), 10 + 3);
            // CHECK(std::ranges::equal(std::span(arr.begin() + 3, arr.end()), std::span(init.begin(), init.end())));
        }

    TEST_CASE("Compare to standard container") {
        auto init = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        std::array<int, 10> test_data{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        dyn_array<int, 10, uint16_t> arr;
        std::vector<int> vec;
        auto check_eq = [&arr, &vec] {
            CHECK_EQ(arr.size(), vec.size());
            for (size_t i = 0; i < arr.size(); ++i) {
                CHECK_EQ(arr[i], vec[i]);
            }
        };
        SUBCASE("push_back") {
            for (auto e: test_data) {
                arr.push_back(e);
                vec.push_back(e);
                check_eq();
            }
        }
        SUBCASE("emplace_back") {
            for (auto e: test_data) {
                arr.emplace_back(e);
                vec.emplace_back(e);
                check_eq();
            }
        }
        SUBCASE("insert at 0") {
            for (int i = 1; i < 10; ++i) {
                arr.insert(arr.begin(), i);
                vec.insert(vec.begin(), i);
                check_eq();
            }
        }

        SUBCASE("erase range") {
            {
                dyn_array<int, 10, uint16_t> a(init);
                a.erase(a.begin(), a.end());
                CHECK(a.empty());
            } {
                dyn_array<int, 10, uint16_t> a(init);
                REQUIRE(a.size() == 10);
                a.erase(a.begin(), a.begin() + 5);
                CHECK_EQ(a.size(), 5);
                CHECK_EQ(a.front(), 6);
                CHECK_EQ(a.back(), 10);
            } {
                dyn_array<int, 10, uint16_t> a(init);
                REQUIRE(a.size() == 10);
                a.erase(a.begin() + 5, a.end());
                CHECK_EQ(a.size(), 5);
                CHECK_EQ(a.front(), 1);
                CHECK_EQ(a.back(), 5);
            } {
                dyn_array<int, 10, uint16_t> a(init);
                REQUIRE(a.size() == 10);
                a.erase(a.begin() + 2, a.end() - 2);
                CHECK_EQ(a.size(), 4);
                CHECK_EQ(a.front(), 1);
                CHECK_EQ(a.back(), 10);
            }
        }
        SUBCASE("random actions") {
            auto random_location = [rnd=std::mt19937{std::random_device{}()}](auto &container) mutable {
                auto max_index = container.size() == 0 ? 0 : container.size() - 1;
                auto dist = std::uniform_int_distribution<size_t>(0, max_index);
                return dist(rnd);
            };
            auto random_action = [rnd=std::mt19937{std::random_device{}()}
                    ](const std::initializer_list<char> &actions) mutable {
                auto dist = std::uniform_int_distribution<size_t>(0, actions.size() - 1);
                return *(actions.begin() + dist(rnd));
            };
            check_eq();
            for (auto i = 0; i < 1000; ++i) {
                auto loc = random_location(vec);
                auto arr_loc = arr.begin() + loc;
                auto vec_loc = vec.begin() + loc;
                auto a = random_action({'i', 'i', 'i', 'e'});
                switch (a) {
                    case 'i':
                        if (arr.capacity() > arr.size()) {
                            arr.insert(arr_loc, i);
                            vec.insert(vec_loc, i);
                        }
                        break;
                    case 'e':
                        if (vec.size() > loc) {
                            arr.erase(arr_loc);
                            vec.erase(vec_loc);
                        }
                        break;
                    default:
                        DOCTEST_FAIL("Unexpected action");
                }
                check_eq();
            }
        }
    }

    TEST_CASE("resize") {
        dyn_array<TestClass, 10, uint16_t> arr = {TestClass{5}, TestClass{6}, TestClass{7}};
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