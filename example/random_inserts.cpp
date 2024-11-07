//
// Created by arnoldm on 07.11.24.
//
#include <chrono>
#include <random>
#include <string>
#include <format>
#include <iostream>
#include <sstream>
#include <map>
#include "btree.h"
#include "testclass.h"

template<typename Container, typename Key, typename Value>
concept has_key_and_value_args = requires(Container c, Key k, Value v)
{
    c.insert(k, v);
};
template<typename Container, typename Key, typename Value>
concept has_pair_args = requires(Container c, Key k, Value v)
{
    c.insert(std::make_pair(k, v));
};

template<typename C>
auto test(C &container, std::ostream& out, size_t N) -> std::tuple<double, double> {
    std::mt19937_64 rng{123};
    std::uniform_int_distribution<unsigned> dist(1U, 1'000'000U);

    auto t1 = std::chrono::high_resolution_clock::now();

    // insert random keys
    for (auto i = 0; i < N; ++i) {
        auto value = dist(rng);
        if constexpr (has_key_and_value_args<C, TestClass, std::string>)
            container.insert(TestClass{value}, std::format("{:04d}", value));
        else if constexpr (has_pair_args<C, TestClass, std::string>)
            container.insert(std::make_pair(TestClass{value}, std::format("{:04d}", value)));
        else
            assert(("cannot work with this container type", false));
    }
    auto t2 = std::chrono::high_resolution_clock::now();
    // print in order
    unsigned previous_value = (*container.begin()).first.value_;
    for(const auto &[key, str_value] : container) {
        long int difference = key.value_ - previous_value;
        std::println(out, "K: {:>10} V: {:>7} D; {:3d}",
            static_cast<std::string>(key), str_value, difference);
        previous_value = key.value_;
    }
    auto t3 = std::chrono::high_resolution_clock::now();
    return std::make_tuple(std::chrono::duration<double>(t2 - t1).count(), std::chrono::duration<double>(t3 - t2).count());
}

void print_time(double map_insertion, double map_reading, double btree_insertion, double btree_reading) {
    std::println(std::cout, "{:>20}  : {:^7} | {:^7} | {:^7}", "Duration", "map", "btree", "btree/map");
    std::println(std::cout, "{:>20}  : {:6.3f}s | {:6.3f}s | {:5.1f}%", "insertion",
                 map_insertion, btree_insertion, btree_insertion / map_insertion * 100.0);
    std::println(std::cout, "{:>20}  : {:6.3f}s | {:6.3f}s | {:5.1f}%", "reading",
                 map_reading, btree_reading, btree_reading / map_reading * 100.0);
}

int main(int argc, char *argv[]) {
    static constexpr size_t N = 1'000'000;

    using btree_type = bt::btree<TestClass, std::string, unsigned, 64>;
    btree_type tree;
    std::ostringstream btree_out;

    using map_type = std::multimap<TestClass, std::string>;
    map_type map;
    std::ostringstream map_out;

    auto [btree_insertion, btree_reading] = test(tree, btree_out, N);
    auto [map_insertion, map_reading] = test(map, map_out, N);

    bool equal = btree_out.view() == map_out.view();
    std::println(std::cout, "tree == map => {:}", equal);
    print_time(map_insertion, map_reading, btree_insertion, btree_reading);
    // std::println(std::cout, "Duration btree: {:6.3f}s ({:5.1f}% of map)", d_btree.count(), d_btree.count() / d_map.count() * 100.0);
}
