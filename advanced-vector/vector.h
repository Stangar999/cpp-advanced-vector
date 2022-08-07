#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }
    RawMemory(const RawMemory&) = delete;

    RawMemory(RawMemory&& other) noexcept {
        Swap(other);
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory& operator=(RawMemory&& other) noexcept {
        Swap(other);
        return *this;
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() noexcept = default;

    explicit Vector(size_t size)
        :data_(size)
        ,size_(size)
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
    }

    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(other.size_)
    {
        other.size_ = 0;
    }

    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    Vector& operator=(const Vector& other) {
        if (this != &other) {
            if (data_.Capacity() < other.size_) {
                Vector copy(other);
                Swap(copy);
            } else {
                if (size_ >= other.size_) {
                    for(size_t i = 0; i < other.size_; ++i) {
                        data_[i] = other[i];
                    }
                    std::destroy_n((data_.GetAddress() + other.size_), (size_ - other.size_));
                    size_ = other.size_;
                } else {
                    for(size_t i = 0; i < size_; ++i) {
                        data_[i] = other[i];
                    }
                    std::uninitialized_copy_n((other.data_.GetAddress() + size_)
                                              ,(other.size_- size_)
                                              ,(data_.GetAddress() + size_));
                    size_ = other.size_;
                }
            }
        }
        return *this;
    }

    Vector& operator=(Vector&& other) noexcept {
        data_ = std::move(other.data_);
        size_ = other.size_;
        other.size_ = 0;
        return *this;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    void Resize(size_t new_size) {
        if(new_size < size_) {
            std::destroy_n((data_.GetAddress() + new_size), (size_ - new_size));
        } else if (new_size > size_){
            Reserve(new_size);
            std::uninitialized_value_construct_n((data_.GetAddress() + size_)
                                                 ,(new_size - size_));
        }
        size_ = new_size;
    }

    template<typename S>
    void PushBack(S&& value) {
        EmplaceBack(std::forward<S>(value));
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        T* ptr_new_elem = nullptr;
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            ptr_new_elem = new (new_data + size_) T(std::forward<Args>(args)...);
            InitUninitData(data_.GetAddress(), size_, new_data.GetAddress());
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            ptr_new_elem = new (data_ + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *ptr_new_elem;
    }

    void PopBack() noexcept {
        std::destroy_n((data_.GetAddress() + (--size_)), 1);
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        InitUninitData(data_.GetAddress(), size_, new_data.GetAddress());
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }
    iterator end() noexcept {
        return (data_ + size_);
    }
    const_iterator begin() const noexcept  {
        return const_cast<Vector*>(this)->begin();
    }
    const_iterator end() const noexcept {
        return const_cast<Vector*>(this)->end();
    }
    const_iterator cbegin() const noexcept {
        return begin();
    }
    const_iterator cend() const noexcept {
        return end();
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args) {
        size_t i_pos = std::distance(cbegin(), pos);
        T* ptr_pos_elem = std::next(begin(), i_pos);
        T* ptr_new_elem = nullptr;
        if (size_ == Capacity()) {
            RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
            ptr_new_elem = new (new_data + i_pos) T(std::forward<Args>(args)...);
            try {
                InitUninitData(begin(), i_pos, new_data.GetAddress());
            } catch(...){
                std::destroy_n(ptr_new_elem, 1);
            }
            try {
                InitUninitData(ptr_pos_elem, std::distance(pos, cend()), std::next(ptr_new_elem));
            } catch(...){
                std::destroy_n(new_data.GetAddress(), i_pos + 1);
            }
            std::destroy_n(data_.GetAddress(), size_);
            data_.Swap(new_data);
        } else {
            if (pos != end()) {
                T tmp_new_elem(std::forward<Args>(args)...);
                new (end()) T(std::move(*std::prev(end())));
                std::move_backward(ptr_pos_elem, std::prev(end()), end());
                *ptr_pos_elem = std::move(tmp_new_elem);
                ptr_new_elem = ptr_pos_elem;
            } else {
               ptr_new_elem = new (ptr_pos_elem) T(std::forward<Args>(args)...);
            }
        }
        ++size_;
        return ptr_new_elem;
    }

    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/ {
        size_t i_pos = std::distance(cbegin(), pos);
        T* ptr_pos_elem = std::next(begin(), i_pos);
        std::move(std::next(ptr_pos_elem), end(), ptr_pos_elem);
        std::destroy_n(std::next(end()), 1);
        --size_;
        return ptr_pos_elem;
    }

    iterator Insert(const_iterator pos, const T& value) {
        return Emplace(pos, value);
    }

    iterator Insert(const_iterator pos, T&& value) {
        return Emplace(pos, std::move(value));
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;

    void InitUninitData(iterator source_pos, std::size_t len,
                      iterator destion_pos)
    {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(source_pos, len, destion_pos);
        } else {
            std::uninitialized_copy_n(source_pos, len, destion_pos);
        }
    }
};
