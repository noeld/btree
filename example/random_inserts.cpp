//
// Created by arnoldm on 07.11.24.
//
#include <random>
#include <string>
#include <format>
#include <iostream>

#include "btree.h"
#include "testclass.h"

int main(int argc, char *argv[]) {
    static constexpr size_t N = 10000;
    std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<unsigned> dist(1U, 1'000'000U);

    using btree_type = bt::btree<TestClass, std::string, unsigned, 64>;
    btree_type tree;

    // insert random keys
    for (auto i = 0; i < N; ++i) {
        auto value = dist(rng);
        tree.insert(TestClass{value}, std::format("{:04d}", value));
    }

    // print in order
    unsigned previous_value = (*tree.begin()).first.value_;
    for(auto const & e : tree) {
        int difference = e.first.value_ - previous_value;
        std::println(std::cout, "K: {:>10} V: {:>7} D; {:3d}",
            static_cast<std::string>(e.first), e.second, difference);
        previous_value = e.first.value_;
    }
}
