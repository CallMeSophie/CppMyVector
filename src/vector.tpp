#pragma once

template <typename T>
MyVector<T>::MyVector(std::size_t n) {
    init_n(n);
}

template <typename T>
MyVector<T>::MyVector(std::size_t n, const T& val) {
    init_n(n, val);
}

template <typename T>
MyVector<T>::MyVector(std::initializer_list<T> il) {
    if (il.size() == 0)
        return;

    data_ = allocate(il.size());
    try {
        for (const T& value : il) {
            new (data_ + size_) T(value);
            ++size_;
        }
    }
    catch (...) {
        destroy_();
        throw;
    }
    capacity_ = size_;
}

template <typename T>
MyVector<T>::MyVector(const MyVector& other) { //copy
    if (other.size_ == 0)
        return;

    data_ = allocate(other.size_);
    try {
        for (; size_ < other.size_; ++size_) {
            new (data_ + size_) T(other.data_[size_]);
        }
    }
    catch (...) {
        destroy_();
        throw;
    }
    capacity_ = size_;
}

template <typename T>
MyVector<T>::MyVector(MyVector&& other) noexcept { //move
    data_ = std::exchange(other.data_, nullptr);
    size_ = std::exchange(other.size_, 0);
    capacity_ = std::exchange(other.capacity_, 0);
}

template <typename T>
MyVector<T>& MyVector<T>::operator=(const MyVector& other) {
    MyVector<T> tmp(other);
    swap(tmp);

    return *this;
}

template <typename T>
MyVector<T>& MyVector<T>::operator=(MyVector&& other) noexcept {
    MyVector<T> tmp(std::move(other));
    swap(tmp);

    return *this;
}

template <typename T>
MyVector<T>::~MyVector() {
    destroy_();
}


template <typename T>
void MyVector<T>::swap(MyVector& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}

template <typename T>
T& MyVector<T>::at(std::size_t i) {
    if (i >= size_) {
        throw std::out_of_range("MyVector::at: index out of range");
    }
    return data_[i];
}

template <typename T>
const T& MyVector<T>::at(std::size_t i) const {
    if (i >= size_) {
        throw std::out_of_range("MyVector::at: index out of range");
    }
    return data_[i];
}

template <typename T>
T& MyVector<T>::front() {
    return data_[0];
}

template <typename T>
const T& MyVector<T>::front() const {
    return data_[0];
}

template <typename T>
T& MyVector<T>::back() {
    return data_[size_ - 1];
}

template <typename T>
const T& MyVector<T>::back() const {
    return data_[size_ - 1];
}

// ---- capacity ----
template <typename T>
void MyVector<T>::reserve(std::size_t new_cap) {
    if (new_cap <= capacity_)
        return;

    T* new_data = allocate(new_cap);
    size_t old_size = size_;
    size_t elems_done = 0;
    try {
        for (; elems_done < size_; ++elems_done) {
            new (new_data + elems_done) T(std::move_if_noexcept(data_[elems_done]));
        }
    }
    catch (...) {
        for (size_t iter = 0; iter < elems_done; iter++) {
            (new_data + iter)->~T();
        }
        ::operator delete(new_data);
        throw;
    }
    destroy_();

    data_ = new_data;
    size_ = old_size;
    capacity_ = new_cap;
}

template <typename T>
void MyVector<T>::shrink_to_fit() {
    if (size_ == capacity_)
        return;
    T* new_data = allocate(size_);
    size_t elems_done = 0;
    try {
        for (; elems_done < size_; ++elems_done) {
            new (new_data + elems_done) T(std::move_if_noexcept(data_[elems_done]));
        }
    }
    catch (...) {
        for (size_t iter = 0; iter < elems_done; iter++) {
            (new_data + iter)->~T();
        }
        ::operator delete(new_data);
        throw;
    }
    for (size_t iter = 0; iter < size_; ++iter) {
        (data_ + iter)->~T();
    }
    ::operator delete(data_);
    capacity_ = size_;
    data_ = new_data;
}

// ---- modifiers ----
template <typename T>
void MyVector<T>::push_back(const T& v){
    emplace_back(v);
}

template <typename T>
void MyVector<T>::push_back(T&& v){
    emplace_back(std::move(v));
}

template <typename T>
template <typename... Args>
T& MyVector<T>::emplace_back(Args&&... args){
    T new_elem(std::forward<Args>(args)...);
    if (size_ >= capacity_) {
        reserve(capacity_ ? capacity_ * 2 : 1);
    }
    new (data_ + size_) T(std::move_if_noexcept(new_elem));
    return data_[size_++];
}

template <typename T>
void MyVector<T>::pop_back(){
    if (size_ > 0) {
        --size_;
        (data_ + size_)->~T();
    }
}

template <typename T>
void MyVector<T>::clear() noexcept{
    while (size_) {
        --size_;
        (data_ + size_)->~T();
    }
}

template <typename T>
void MyVector<T>::resize(std::size_t n){
    if (size_ == n) {
        return;
    }
    while (size_ > n) { //we have too many elems, delete elems from end
        --size_;
        (data_ + size_)->~T();
    }
    if (n > capacity_) { //need bigger buffer
        reserve(n);
    }
    while (size_ < n) { //too few elems, add new elems at the end
        //no need to try catch this cause data stays valid even if thrown?
        new (data_ + size_) T();
        ++size_;
    }
}

template <typename T>
T* MyVector<T>::insert(const_iterator pos, const T& v){
    if (pos < begin() || pos > end()) {
        throw std::out_of_range("MyVector::insert pos out of range");
    }

    std::size_t index = pos - begin();
    T new_elem(v);

    if (index == size_) {
        push_back(new_elem);
    }
    else {
        if (size_ == capacity_) {
            reserve(capacity_ ? capacity_ * 2 : 1);
        }

        new (data_ + size_) T(std::move(data_[size_ - 1]));
        ++size_;

        for (size_t iter = size_ - 2; iter > index; --iter) {
            data_[iter] = std::move(data_[iter - 1]);
        }

        data_[index] = std::move(new_elem);
    }
    
    return data_ + index;
}

template <typename T>
T* MyVector<T>::erase(const_iterator pos) {
    if (pos < begin() || pos >= end()) {
        throw std::out_of_range("MyVector::erase pos out of range");
    }

    size_t index = pos - begin();

    for (size_t iter = index; iter + 1 < size_; ++iter) {
        data_[iter] = std::move(data_[iter + 1]);
    }
    
    (data_ + size_ - 1)->~T();
    --size_;

    return data_ + index;
}

template <typename T>
template <typename... Args>
void MyVector<T>::init_n(std::size_t n, const Args&... args) {
    if (n == 0)
        return;

    data_ = allocate(n);
    try {
        for (; size_ < n; ++size_) {
            new (data_ + size_) T(args...);
        }
    }
    catch (...) {
        destroy_();
        throw;
    }
    capacity_ = n;
}

template <typename T>
void MyVector<T>::destroy_() {
    clear();
    ::operator delete(data_);
    data_ = nullptr;
    capacity_ = 0;
}

template <typename T>
T* MyVector<T>::allocate(size_t n) {
    return static_cast<T*>(::operator new(n * sizeof(T)));
}
