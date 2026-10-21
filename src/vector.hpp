#pragma once
#include <cstddef>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T>
class MyVector {
public:
    using value_type     = T;
    using iterator       = T*;
    using const_iterator = const T*;

    // ---- construction / destruction (TODO) ----
    MyVector() noexcept = default;
    explicit MyVector(std::size_t n);
    MyVector(std::size_t n, const T& val);
    MyVector(std::initializer_list<T> il);
    MyVector(const MyVector& other); //copy
    MyVector(MyVector&& other) noexcept; //move
    MyVector& operator=(const MyVector& other);
    MyVector& operator=(MyVector&& other) noexcept;
    ~MyVector();

    // ---- element access ----
    T&       operator[](std::size_t i)       { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }
    T&       at(std::size_t i);
    const T& at(std::size_t i) const;
    T&       front();
    const T& front() const;
    T&       back();
    const T& back() const;
    T*       data() noexcept       { return data_; }
    const T* data() const noexcept { return data_; }

    // ---- capacity ----
    std::size_t size()     const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
    bool        empty()    const noexcept { return size_ == 0; }
    void reserve(std::size_t new_cap);
    void shrink_to_fit();                    // TODO

    // ---- modifiers ----
    void push_back(const T& v);              // TODO
    void push_back(T&& v);                   // TODO
    template <typename... Args>
    T& emplace_back(Args&&... args);         // TODO (push_back can call this)
    void pop_back();                         // TODO
    void clear() noexcept;                   // TODO: destroy, keep capacity
    void resize(std::size_t n);
    iterator insert(const_iterator pos, const T& v);  // TODO
    iterator erase(const_iterator pos);      // TODO
    void swap(MyVector& other) noexcept;

    // ---- iterators ----
    iterator       begin() noexcept       { return data_; }
    iterator       end() noexcept         { return data_ + size_; }
    const_iterator begin() const noexcept { return data_; }
    const_iterator end() const noexcept   { return data_ + size_; }

private:
    T*          data_     = nullptr;
    std::size_t size_     = 0;
    std::size_t capacity_ = 0;

    template <typename... Args>
    void init_n(std::size_t n, const Args&... args);
    void destroy_();
    T* allocate(size_t n);

    // Hints for private helpers:
    //  - allocate(n):   static_cast<T*>(::operator new(n * sizeof(T)))
    //  - reallocate(n): allocate, move_if_noexcept each element via placement new,
    //                   destroy old ones, ::operator delete old buffer
    //  - grow():        new_cap = capacity_ ? capacity_ * 2 : 4
    //  - careful: push_back(v) where v aliases an element of *this
};

#include "vector.tpp"