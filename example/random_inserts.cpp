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
    for (size_t i = 0; i < N; ++i) {
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

void print_time(double map_insertion, double map_reading, double btree_insertion, double btree_reading,
                double botree_insertion, double botree_reading) {
    std::println(std::cout, "{:>20}  : {:^12} | {:^12} | {:^12} | {:^12} | {:^12}", "Duration", "std::map", "btree", "btree/map", "botree", "botree/map");
    std::println(std::cout, "{:>20}  : {:11.3f}s | {:11.3f}s | {:11.1f}% | {:11.3f}s | {:11.1f}%", "insertion",
                 map_insertion, btree_insertion, btree_insertion / map_insertion * 100.0,
                 botree_insertion, botree_insertion / map_insertion * 100.0);
    std::println(std::cout, "{:>20}  : {:11.3f}s | {:11.3f}s | {:11.1f}% | {:11.3f}s | {:11.1f}%", "reading",
                 map_reading, btree_reading, btree_reading / map_reading * 100.0,
                 botree_reading, botree_reading / map_reading * 100.0);
}

int main(int argc, char *argv[]) {
    static constexpr size_t N = 1'000'000;

    using btree_type = bt::btree<TestClass, std::string, uint16_t, 51, 88>;
    std::println(std::cout, "sizeof(internal_node_type<Order {}>) = {:8}", btree_type::internal_node_type::order(), sizeof(btree_type::internal_node_type));
    std::println(std::cout, "sizeof(leaf_node_type<Order {}>)     = {:8}", btree_type::leaf_node_type::order(), sizeof(btree_type::leaf_node_type));

    btree_type tree;
    std::ostringstream btree_out;

    using map_type = std::multimap<TestClass, std::string>;
    map_type map;
    std::ostringstream map_out;

    static constexpr std::size_t PAGE_SIZE = 4096;
    constexpr auto best_internal_order = bt::best_order<bt::btree_internal_node, TestClass, std::string, uint16_t, PAGE_SIZE>();
    constexpr auto best_leaf_order = bt::best_order<bt::btree_leaf_node, TestClass, std::string, uint16_t, PAGE_SIZE>();
    using best_btree_order_type = bt::btree<TestClass, std::string, uint16_t, best_internal_order, best_leaf_order>;
    best_btree_order_type botree;
    std::ostringstream botree_out;

    std::println(std::cout, "Best order for internal nodes for page size {:4}: Order {:4} = {} bytes",
        PAGE_SIZE, best_internal_order, sizeof(best_btree_order_type::internal_node_type));
    std::println(std::cout, "Best order for leaf nodes for     page size {:4}: Order {:4} = {} bytes",
        PAGE_SIZE, best_leaf_order, sizeof(best_btree_order_type::leaf_node_type));

    auto [btree_insertion, btree_reading] = test(tree, btree_out, N);
    auto [map_insertion, map_reading] = test(map, map_out, N);
    auto [botree_insertion, botree_reading] = test(botree, botree_out, N);

    bool equal = btree_out.view() == map_out.view();
    std::println(std::cout, "output of btree == map => {}", equal ? "equal ☑️" : "!! not equal 🫣 !!");
    bool boequal = botree_out.view() == map_out.view();
    std::println(std::cout, "output of botree == map => {}", boequal ? "equal ☑️" : "!! not equal 🫣 !!");
    print_time(map_insertion, map_reading, btree_insertion, btree_reading, botree_insertion, botree_reading);
    std::println(std::cout, "Test with {:L} key/values (TestClass/std::string)", N);

}
