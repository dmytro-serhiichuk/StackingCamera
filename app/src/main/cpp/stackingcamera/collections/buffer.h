//
// Created by sedv2 on 11.01.2026.
//

#ifndef STACKINGCAMERA_BUFFER_H
#define STACKINGCAMERA_BUFFER_H

#include <cstdint>
#include <stdexcept>
#include <cstring>

template <typename T>
class Buffer {
public:
    size_t capacity;
    size_t size;
    T* buffer;

    Buffer(size_t _capacity = 0) {
        buffer = _capacity > 0 ? new T[_capacity] : nullptr;
        capacity = _capacity;
        size = 0;
    }
    ~Buffer() {
        delete [] buffer;
        buffer = nullptr;
        size = 0;
        capacity = 0;
    }

    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            Buffer temp(other);
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

#endif //STACKINGCAMERA_BUFFER_H
