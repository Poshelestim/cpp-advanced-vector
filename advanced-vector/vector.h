#pragma once
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept;

    RawMemory& operator=(RawMemory &&rhs) noexcept;

    explicit RawMemory(size_t capacity);

    ~RawMemory();

    T *operator+(size_t offset) noexcept;

    const T *operator+(size_t offset) const noexcept;

    const T &operator[](size_t index) const noexcept;

    T &operator[](size_t index) noexcept;

    void Swap(RawMemory &other) noexcept;

    const T *GetAddress() const noexcept;

    T *GetAddress() noexcept;

    [[nodiscard]] size_t Capacity() const;

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T *Allocate(size_t n);

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept;

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template<typename T>
RawMemory<T>::RawMemory(RawMemory<T> &&other) noexcept
{
    Deallocate(buffer_);
    buffer_ = std::exchange(other.buffer_, nullptr);
    capacity_ = std::exchange(other.capacity_, 0);
}

template<typename T>
RawMemory<T> &RawMemory<T>::operator=(RawMemory &&rhs) noexcept
{
    Deallocate(buffer_);
    buffer_ = std::exchange(rhs.buffer_, nullptr);
    capacity_ = std::exchange(rhs.capacity_, 0);
    return *this;
}

template<typename T>
RawMemory<T>::RawMemory(size_t capacity)
    : buffer_(Allocate(capacity))
    , capacity_(capacity)
{

}

template<typename T>
RawMemory<T>::~RawMemory()
{
    Deallocate(buffer_);
}

template<typename T>
T *RawMemory<T>::operator+(size_t offset) noexcept
{
    // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
    assert(offset <= capacity_);
    return buffer_ + offset;
}

template<typename T>
const T *RawMemory<T>::operator+(size_t offset) const noexcept
{
    return const_cast<RawMemory&>(*this) + offset;
}

template<typename T>
const T &RawMemory<T>::operator[](size_t index) const noexcept
{
    return const_cast<RawMemory&>(*this)[index];
}

template<typename T>
T &RawMemory<T>::operator[](size_t index) noexcept
{
    assert(index < capacity_);
    return buffer_[index];
}

template<typename T>
void RawMemory<T>::Swap(RawMemory &other) noexcept
{
    std::swap(buffer_, other.buffer_);
    std::swap(capacity_, other.capacity_);
}

template<typename T>
const T *RawMemory<T>::GetAddress() const noexcept
{
    return buffer_;
}

template<typename T>
T *RawMemory<T>::GetAddress() noexcept
{
    return buffer_;
}

template<typename T>
size_t RawMemory<T>::Capacity() const
{
    return capacity_;
}

template<typename T>
T *RawMemory<T>::Allocate(size_t n)
{
    return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
}

template<typename T>
void RawMemory<T>::Deallocate(T *buf) noexcept
{
    operator delete(buf);
}

template <typename T>
class Vector
{
public:

    Vector() = default;

    explicit Vector(size_t size);

    ~Vector();

    Vector(const Vector &other);

    Vector(Vector &&other) noexcept;

    Vector &operator=(const Vector& rhs);

    Vector &operator=(Vector &&rhs) noexcept;

    void Swap(Vector &other) noexcept;

    [[nodiscard]] size_t Size() const noexcept;

    [[nodiscard]] size_t Capacity() const noexcept;

    const T &operator[](size_t index) const noexcept;

    T &operator[](size_t index) noexcept;

    using iterator = T*;
    using const_iterator = const T*;

    iterator begin() noexcept;
    iterator end() noexcept;
    const_iterator begin() const noexcept;
    const_iterator end() const noexcept;
    const_iterator cbegin() const noexcept;
    const_iterator cend() const noexcept;

    void Reserve(size_t new_capacity);

    void Resize(size_t new_size);

    void PushBack(const T &value);

    void PushBack(T &&value);

    void PopBack() noexcept;

    template <typename... Args>
    T &EmplaceBack(Args &&...args);

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args);
    iterator Erase(const_iterator pos) /*noexcept(std::is_nothrow_move_assignable_v<T>)*/;
    iterator Insert(const_iterator pos, const T& value);
    iterator Insert(const_iterator pos, T&& value);

private:

    RawMemory<T> data_;
    size_t size_ = 0;
};

template<typename T>
Vector<T>::Vector(size_t size) : data_(size), size_(size)
{
    std::uninitialized_value_construct_n(data_.GetAddress(), size);
}

template<typename T>
Vector<T>::~Vector()
{
    std::destroy_n(data_.GetAddress(), size_);
}

template<typename T>
Vector<T>::Vector(const Vector &other) : data_(other.size_), size_(other.size_)
{
    std::uninitialized_copy_n(other.data_.GetAddress(), size_, data_.GetAddress());
}

template<typename T>
Vector<T>::Vector(Vector &&other) noexcept
{
    Swap(other);
}

template<typename T>
Vector<T> &Vector<T>::operator=(const Vector &rhs)
{
    if (this != &rhs)
    {
        if (rhs.size_ > data_.Capacity())
        {
            Vector rhs_copy(rhs);
            Swap(rhs_copy);
        }
        else
        {
            /* Скопировать элементы из rhs, создав при необходимости новые
                       или удалив существующие */
            size_t copy_pos = rhs.size_ < size_ ? rhs.size_ : size_;
            auto end_copy_pos = std::copy_n(rhs.data_.GetAddress(),
                                            copy_pos,
                                            data_.GetAddress());
            if (rhs.size_ < size_)
            {
                std::destroy_n(end_copy_pos, size_ - rhs.size_);
            }
            else
            {
                std::uninitialized_copy_n(rhs.data_.GetAddress() + size_,
                                          rhs.size_ - size_,
                                          end_copy_pos);
            }
            size_ = rhs.size_;
        }
    }
    return *this;
}

template<typename T>
Vector<T> &Vector<T>::operator=(Vector<T> &&rhs) noexcept
{
    Swap(rhs);
    return *this;
}

template<typename T>
void Vector<T>::Swap(Vector &other) noexcept
{
    data_.Swap(other.data_);
    std::swap(size_, other.size_);
}

template<typename T>
size_t Vector<T>::Size() const noexcept
{
    return size_;
}

template<typename T>
size_t Vector<T>::Capacity() const noexcept
{
    return data_.Capacity();
}

template<typename T>
const T &Vector<T>::operator[](size_t index) const noexcept
{
    return const_cast<Vector&>(*this)[index];
}

template<typename T>
T &Vector<T>::operator[](size_t index) noexcept
{
    assert(index < size_);
    return data_[index];
}

template<typename T>
typename Vector<T>::iterator Vector<T>::begin() noexcept
{
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::iterator Vector<T>::end() noexcept
{
    return data_.GetAddress() + size_;
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::begin() const noexcept
{
    return cbegin();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::end() const noexcept
{
    return cend();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cbegin() const noexcept
{
    return data_.GetAddress();
}

template<typename T>
typename Vector<T>::const_iterator Vector<T>::cend() const noexcept
{
    return data_.GetAddress() + size_;
}

template<typename T>
void Vector<T>::Reserve(size_t new_capacity)
{
    if (new_capacity <= data_.Capacity())
    {
        return;
    }

    RawMemory<T> new_data(new_capacity);
    if constexpr ( std::is_nothrow_move_constructible_v<T> ||
            !std::is_copy_constructible_v<T> )
    {
        std::uninitialized_move_n(data_.GetAddress(),
                                  size_,
                                  new_data.GetAddress());
    }
    else
    {
        std::uninitialized_copy_n(data_.GetAddress(),
                                  size_,
                                  new_data.GetAddress());
    }
    std::destroy_n(data_.GetAddress(), size_);
    data_.Swap(new_data);
}

template<typename T>
void Vector<T>::Resize(size_t new_size)
{
    if (new_size == size_)
    {
        return;
    }

    if (new_size < size_)
    {
        std::destroy_n(data_.GetAddress() + new_size, size_ - new_size);
    }
    else
    {
        Reserve(new_size);
        std::uninitialized_value_construct_n(data_.GetAddress() + size_,
                                             new_size - size_);
    }

    size_ = new_size;
}

template<typename T>
void Vector<T>::PushBack(const T &value)
{
    EmplaceBack(value);
}

template<typename T>
void Vector<T>::PushBack(T &&value)
{
    EmplaceBack(std::move(value));
}

template<typename T>
void Vector<T>::PopBack() noexcept
{
    if (size_ != 0U)
    {
        std::destroy_at(data_.GetAddress() + size_ - 1);
        --size_;
    }
}

template <typename T>
template <typename... Args>
T &Vector<T>::EmplaceBack(Args &&...args)
{
    if (size_ == Capacity())
    {
        RawMemory<T> new_data(size_ == 0 ? 1 : size_ * 2);
        new (new_data + size_) T(std::forward<Args>(args) ...);
        if constexpr ( std::is_nothrow_move_constructible_v<T> ||
                !std::is_copy_constructible_v<T> )
        {
            std::uninitialized_move_n(data_.GetAddress(),
                                      size_,
                                      new_data.GetAddress());
        }
        else
        {
            std::uninitialized_copy_n(data_.GetAddress(),
                                      size_,
                                      new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
        data_.Swap(new_data);
    }
    else
    {
        new (data_.GetAddress() + size_) T(std::forward<Args>(args) ...);
    }
    ++size_;
    return data_[size_ - 1];
}

template <typename T>
template <typename... Args>
typename Vector<T>::iterator Vector<T>::Emplace(const_iterator pos, Args &&...args)
{
    size_t index = std::distance(cbegin(), pos);

    if (size_ == data_.Capacity())
    {
        size_t count_before = std::distance(cbegin(), pos);
        size_t count_after = std::distance(pos, cend());

        RawMemory<T> tmp((size_ == 0) ? 1 : size_ * 2);
        new (tmp.GetAddress() + index) T(std::forward<Args>(args)...);

        try
        {
            if constexpr ( std::is_nothrow_move_constructible_v<T> ||
                    !std::is_copy_constructible_v<T> )
            {
                std::uninitialized_move_n(begin(), count_before, tmp.GetAddress());
                std::uninitialized_move_n(begin() + count_before, count_after, tmp.GetAddress() + count_before + 1);
            }
            else
            {
                std::uninitialized_copy_n(begin(), count_before, tmp.GetAddress());
                std::uninitialized_copy_n(begin() + count_before, count_after, tmp.GetAddress() + count_before + 1);
            }
        }
        catch (...)
        {
            std::destroy_n(tmp.GetAddress(), index + 1);
            throw;
        }

        std::destroy_n(begin(), size_);
        data_.Swap(tmp);
    }
    else
    {
        if (size_ != index)
        {
            new (end()) T(std::forward<T>(*(end() - 1)));
            std::move_backward(begin() + index, end() - 1, end());
            data_[index] = T(std::forward<Args>(args)...);
        }
        else
        {
            new (begin() + index) T(std::forward<Args>(args)...);
        }
    }

    ++size_;
    return begin() + index;
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Erase(const_iterator pos)
{
    assert(size_ > 0);
    auto index = static_cast<size_t>(pos - begin());
    std::move(begin() + index + 1, end(), begin() + index);
    data_[size_ - 1].~T();
    --size_;
    return begin() + index;
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, const T &value)
{
    return Emplace(pos, value);
}

template<typename T>
typename Vector<T>::iterator Vector<T>::Insert(const_iterator pos, T &&value)
{
    return Emplace(pos, std::move(value));
}
