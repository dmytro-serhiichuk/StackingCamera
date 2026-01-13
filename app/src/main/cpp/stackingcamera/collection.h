//
// Created by sedv2 on 11.01.2026.
//

#ifndef STACKINGCAMERA_COLLECTION_H
#define STACKINGCAMERA_COLLECTION_H

#include <cstdint>
#include <stdexcept>
#include <cstring>

template <typename T>
class Collection {
public:
    size_t capacity;
    size_t size;
    T* buffer;

    Collection(size_t _capacity = 0) {
        buffer = _capacity > 0 ? new T[_capacity] : nullptr;
        capacity = _capacity;
        size = 0;
    }
    ~Collection() {
        delete [] buffer;
        buffer = nullptr;
        size = 0;
        capacity = 0;
    }

    Collection& operator=(const Collection& other) {
        if (this != &other) {
            Collection temp(other);
            std::swap(capacity, temp.capacity);
            std::swap(size, temp.size);
            std::swap(buffer, temp.buffer);
        }
        return *this;
    }

    T& operator[](size_t index) {
        if (index >= capacity) throw std::out_of_range("Index out of range");
        return buffer[index];
    }
    const T& operator[](size_t index) const {
        if (index >= capacity) throw std::out_of_range("Index out of range");
        return buffer[index];
    }

    T* begin() {
        return buffer;
    }
    T* end() {
        return buffer + size;
    }

    void shrink() {
        if (size < capacity) {
            T* _buffer = new T[size];
            memcpy(_buffer, buffer, size * sizeof(T));
            delete [] buffer;
            buffer = _buffer;
            capacity = size;
        }
    }
};

#endif //STACKINGCAMERA_COLLECTION_H
