// Build: g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined test_vector.cpp -o test && ./test
#include "vector.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// Tracks live instances to catch leaks / double destruction.
struct Counted {
    static int live;
    int v;
    Counted(int x = 0) : v(x) { ++live; }
    Counted(const Counted& o) : v(o.v) { ++live; }
    Counted(Counted&& o) noexcept : v(o.v) { ++live; }
    Counted& operator=(const Counted&) = default;
    ~Counted() { --live; }
};
int Counted::live = 0;

// Throwing move: MyVector should copy instead of move on reallocation.
struct ThrowingMove {
    static int copies, moves;
    ThrowingMove() = default;
    ThrowingMove(const ThrowingMove&) { ++copies; }
    ThrowingMove(ThrowingMove&&) noexcept(false) { ++moves; }
};
int ThrowingMove::copies = 0;
int ThrowingMove::moves  = 0;

#define RUN(t) do { t(); std::cout << "ok  " #t "\n"; } while (0)

void test_default() {
    MyVector<int> v;
    assert(v.size() == 0 && v.capacity() == 0 && v.empty());
}

void test_ctors() {
    MyVector<int> a(3);
    assert(a.size() == 3 && a[0] == 0);
    MyVector<int> b(4, 7);
    assert(b.size() == 4 && b[3] == 7);
    MyVector<std::string> c{"a", "b", "c"};
    assert(c.size() == 3 && c[2] == "c");
}

void test_push_and_growth() {
    MyVector<int> v;
    std::size_t prev_cap = 0;
    for (int i = 0; i < 1000; ++i) {
        v.push_back(i);
        assert(v.capacity() >= v.size());
        assert(v.capacity() >= prev_cap);
        prev_cap = v.capacity();
    }
    for (int i = 0; i < 1000; ++i) assert(v[i] == i);
}

void test_reserve() {
    MyVector<int> v;
    v.reserve(100);
    assert(v.capacity() >= 100 && v.size() == 0);
    int* p = v.data();
    for (int i = 0; i < 100; ++i) v.push_back(i);
    assert(v.data() == p);           // no reallocation
    v.reserve(10);                   // must not shrink
    assert(v.capacity() >= 100);
}

void test_shrink_clear() {
    MyVector<int> v;
    for (int i = 0; i < 10; ++i) v.push_back(i);
    v.reserve(100);
    v.shrink_to_fit();
    assert(v.capacity() == v.size());
    std::size_t cap = v.capacity();
    v.clear();
    assert(v.size() == 0 && v.capacity() == cap);
}

void test_at() {
    MyVector<int> v{1, 2, 3};
    assert(v.at(2) == 3);
    bool thrown = false;
    try { v.at(3); } catch (const std::out_of_range&) { thrown = true; }
    assert(thrown);
}

void test_front_back_pop() {
    MyVector<int> v{1, 2, 3};
    assert(v.front() == 1 && v.back() == 3);
    v.pop_back();
    assert(v.size() == 2 && v.back() == 2);
}

void test_copy_move() {
    MyVector<std::string> a{"x", "y", "z"};
    MyVector<std::string> b = a;
    b[0] = "changed";
    assert(a[0] == "x" && b[0] == "changed");

    MyVector<std::string> c = std::move(a);
    assert(c.size() == 3 && a.size() == 0);   // moved-from is valid & empty

    MyVector<std::string> d;
    d = c;
    assert(d.size() == 3 && d[1] == "y");
    d = std::move(c);
    assert(d.size() == 3);

    d = d;                                    // self-assignment
    assert(d.size() == 3 && d[2] == "z");
}

void test_move_only() {
    MyVector<std::unique_ptr<int>> v;
    for (int i = 0; i < 50; ++i) v.emplace_back(std::make_unique<int>(i));
    assert(*v[49] == 49);
    MyVector<std::unique_ptr<int>> w = std::move(v);
    assert(*w[10] == 10);
}

void test_lifetimes() {
    {
        MyVector<Counted> v;
        for (int i = 0; i < 100; ++i) v.push_back(Counted(i));
        assert(Counted::live == 100);
        v.pop_back();
        assert(Counted::live == 99);
        v.erase(v.begin());
        assert(Counted::live == 98);
        v.insert(v.begin() + 1, Counted(-1));
        assert(Counted::live == 99 && v[1].v == -1);
        v.resize(10);
        assert(Counted::live == 10);
        v.clear();
        assert(Counted::live == 0);
        v.resize(5);
        MyVector<Counted> copy = v;
        assert(Counted::live == 10);
    }
    assert(Counted::live == 0);   // no leaks after destruction
}

void test_insert_erase() {
    MyVector<int> v{1, 2, 4, 5};
    v.insert(v.begin() + 2, 3);
    assert(v.size() == 5);
    for (int i = 0; i < 5; ++i) assert(v[i] == i + 1);
    auto it = v.erase(v.begin());
    assert(*it == 2 && v.size() == 4);
    v.erase(v.end() - 1);
    assert(v.size() == 3 && v.back() == 4);
}

void test_resize() {
    MyVector<int> v{1, 2, 3};
    v.resize(5);
    assert(v.size() == 5 && v[3] == 0 && v[4] == 0);
    v.resize(2);
    assert(v.size() == 2 && v[1] == 2);
}

void test_aliasing_push_back() {
    MyVector<std::string> v;
    v.push_back("hello");
    for (int i = 0; i < 20; ++i) v.push_back(v[0]);  // v[0] may be invalidated by growth
    for (auto& s : v) assert(s == "hello");
}

void test_move_if_noexcept() {
    ThrowingMove::copies = ThrowingMove::moves = 0;
    MyVector<ThrowingMove> v;
    v.reserve(1);
    v.emplace_back();
    v.emplace_back();   // forces reallocation
    assert(ThrowingMove::copies >= 1 && ThrowingMove::moves == 0);
}

void test_iteration_and_swap() {
    MyVector<int> a{1, 2, 3}, b{9};
    a.swap(b);
    assert(a.size() == 1 && b.size() == 3);
    int sum = 0;
    for (int x : b) sum += x;
    assert(sum == 6);
}

void test_random_vs_std() {
    MyVector<int> mine;
    std::vector<int> ref;
    unsigned s = 12345;
    auto rnd = [&] { s = s * 1103515245u + 12345u; return (s >> 16) & 0x7fff; };
    for (int i = 0; i < 20000; ++i) {
        switch (rnd() % 4) {
            case 0: case 1: { int x = rnd(); mine.push_back(x); ref.push_back(x); break; }
            case 2: if (!ref.empty()) { mine.pop_back(); ref.pop_back(); } break;
            case 3: if (!ref.empty()) {
                std::size_t i2 = rnd() % ref.size();
                mine.erase(mine.begin() + i2);
                ref.erase(ref.begin() + i2);
            } break;
        }
        assert(mine.size() == ref.size());
    }
    for (std::size_t i = 0; i < ref.size(); ++i) assert(mine[i] == ref[i]);
}

int main() {
    RUN(test_default);
    RUN(test_ctors);
    RUN(test_push_and_growth);
    RUN(test_reserve);
    RUN(test_shrink_clear);
    RUN(test_at);
    RUN(test_front_back_pop);
    RUN(test_copy_move);
    RUN(test_move_only);
    RUN(test_lifetimes);
    RUN(test_insert_erase);
    RUN(test_resize);
    RUN(test_aliasing_push_back);
    RUN(test_move_if_noexcept);
    RUN(test_iteration_and_swap);
    RUN(test_random_vs_std);
    std::cout << "all passed\n";
}