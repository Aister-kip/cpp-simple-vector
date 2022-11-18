#pragma once

#include <cassert>
#include <initializer_list>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <iterator>
#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t size)
        : size_(size)
    {}
    size_t size_;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    // Создаёт вектор из size элементов, инициализированных значением по умолчанию
    explicit SimpleVector(size_t size)
        : SimpleVector(size, Type{}) {
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value) {
        ArrayPtr<Type> temp(size);
        std::fill(temp.Get(), temp.Get() + size, value);
        head_.swap(temp);
        size_ = size;
        capacity_ = size;
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        ArrayPtr<Type> temp(init.size());
        std::copy(init.begin(), init.end(), temp.Get());
        head_.swap(temp);
        size_ = init.size();
        capacity_ = init.size();
    }

    SimpleVector(const SimpleVector& other) {
        ArrayPtr<Type> temp(other.GetSize());
        std::copy(other.cbegin(), other.cend(), temp.Get());
        head_.swap(temp);
        size_ = other.GetSize();
        capacity_ = other.GetSize();

    }

    SimpleVector(ReserveProxyObj obj) {
        ArrayPtr<Type> temp(obj.size_);
        std::fill(temp.Get(), temp.Get() + obj.size_, Type{});
        head_.swap(temp);
        capacity_ = obj.size_;
    }

    SimpleVector(SimpleVector&& other) {
        swap(other);
    }

    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            if (!rhs.IsEmpty()) {
                SimpleVector<Type> temp(rhs);
                swap(temp);
            }
            else {
                this->size_ = 0;
            }
        }
        return *this;
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        auto item_copy = item;
        PushBack(std::move(item));
    }

    void PushBack(Type&& item) {
        if (size_ < capacity_) {
            head_[size_] = std::move(item);
            ++size_;
        }
        else {
            ArrayPtr<Type> temp(capacity_ > 0 ? capacity_ * 2 : 1);
            std::move(begin(), end(), temp.Get());
            temp[size_] = std::move(item);
            head_.swap(temp);
            ++size_;
            capacity_ = capacity_ > 0 ? capacity_ * 2 : 1;
        }
    }


    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    Iterator Insert(ConstIterator pos, const Type& value) {
        assert(pos >= begin() && pos <= end());
        auto value_copy = value;
        return Insert(pos, std::move(value));
    }

    Iterator Insert(Iterator pos, Type&& value) {
        assert(pos >= begin() && pos <= end());
        size_t index = std::distance(begin(), pos);
        if (size_ < capacity_) {
            std::move_backward(pos, end(), end() + 1);
            head_[index] = std::move(value);
        }
        else {
            ArrayPtr<Type> temp(capacity_ > 0 ? capacity_ * 2 : 1);
            std::move(begin(), end(), temp.Get());
            std::move_backward(temp.Get() + index, temp.Get() + size_, temp.Get() + size_ + 1);
            temp[index] = std::move(value);
            head_.swap(temp);
            capacity_ = capacity_ > 0 ? capacity_ * 2 : 1;
        }
        ++size_;
        return begin() + index;
    }

    // Удаляет последний элемент вектора. Вектор не должен быть пустым
    void PopBack() noexcept {
        assert(!IsEmpty());
        --size_;
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        assert(pos >= begin() && pos <= end());
        size_t index = std::distance(cbegin(), pos);
        if (!IsEmpty()) {
            std::move(begin() + index + 1, end(), begin() + index);
            --size_;
        }
        return begin() + index;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        head_.swap(other.head_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return !GetSize();
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) {
        using namespace std;
        if (index > size_) {
            throw std::out_of_range("Out of range. Index >= size!"s);
        }
        return head_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const {
        using namespace std;
        if (index > size_) {
            throw std::out_of_range("Out of range. Index >= size!"s);
        }
        return head_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        using namespace std;
        if (index > size_) {
            throw std::out_of_range("Out of range. Index >= size!"s);
        }
        return head_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        return At(index);
    }

    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        size_ = 0;
    }

    void Resize(size_t new_size) {
        if (new_size > capacity_) {
            ArrayPtr<Type> temp(new_size);
            std::move(begin(), end(), temp.Get());
            std::fill(temp.Get() + size_, temp.Get() + new_size, Type{});
            head_.swap(temp);
            capacity_ = new_size;
            size_ = new_size;
        }
        else if (new_size < size_) {
            size_ = new_size;
        }
        else {
            std::fill(begin() + size_, begin() + new_size, Type{});
            size_ = new_size;
        }

    }
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            ArrayPtr<Type> temp(new_capacity);
            std::move(begin(), end(), temp.Get());
            head_.swap(temp);
            capacity_ = new_capacity;
        }
    }

    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        return head_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        return head_.Get() + size_;
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        return cbegin();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        return cend();
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        return head_.Get();
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        return head_.Get() + size_;
    }

private:
    size_t size_ = 0;
    size_t capacity_ = 0;
    ArrayPtr<Type> head_;
};

template <typename Type>
inline bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename Type>
inline bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs == rhs);
}

template <typename Type>
inline bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
inline bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(rhs < lhs);
}

template <typename Type>
inline bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return (rhs < lhs);
}

template <typename Type>
inline bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(lhs < rhs);
}