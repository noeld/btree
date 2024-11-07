//
// Created by arnoldm on 07.11.24.
//

#ifndef TESTCLASS_H
#define TESTCLASS_H

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

    TestClass &operator=(const TestClass &other) {
        if (this == &other)
            return *this;
        value_ = other.value_;
        return *this;
    }

    TestClass &operator=(TestClass &&other) noexcept {
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

    friend std::hash<TestClass>;

    auto operator<=>(const TestClass &other) const {
        return value_ <=> other.value_;
    }

    explicit operator std::string() const { return std::format("TC[{}]", value_); }

    friend std::ostream &operator<<(std::ostream &out, TestClass const &t) {
        return out << t.operator std::string();
    }

    unsigned value_{value_counter_++};
    static unsigned value_counter_;
    static unsigned ctor_counter_;
};

namespace std {
    template<>
    struct hash<TestClass> {
        std::size_t operator()(const TestClass &obj) const noexcept {
            std::size_t seed = 0x6832BA6B;
            seed ^= (seed << 6) + (seed >> 2) + 0x25F0886E + static_cast<std::size_t>(obj.value_);
            return seed;
        }
    };
}

unsigned TestClass::ctor_counter_ = 0;
unsigned TestClass::value_counter_ = 0;

#endif //TESTCLASS_H
