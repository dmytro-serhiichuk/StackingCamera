//
// Created by sedv2 on 15.01.2025.
//

#ifndef IMAGESTACKER_COLLECTION_H
#define IMAGESTACKER_COLLECTION_H

#include <cstdint>
#include <stdexcept>

template <typename T>
class List {
public:
    size_t capacity;
    size_t size;
    T** buffer;

    List(size_t initCapacity = 3) {
        capacity = initCapacity;
        size = 0;
        buffer = new T*[capacity];
    }
    ~List() {
        for (size_t i = 0; i < size; i++) delete buffer[i];
        delete[] buffer;
        buffer = nullptr;
        capacity = 0;
        size = 0;
    }

    void add(T* item) {
        if (size >= capacity) {
            capacity *= 2;
            T** _buffer = new T*[capacity];
            for (size_t i = 0; i < size; ++i) {
                _buffer[i] = buffer[i];
            }
            delete[] buffer;
            buffer = _buffer;
        }
        size++;
        buffer[size - 1] = item;
    }
    void removeAt(size_t index) {
        if (index >= size) return;
        delete buffer[index];
        for (size_t i = index; i < size - 1; i++) {
            buffer[i] = buffer[i + 1];
        }
        size--;
    }
};

#endif //IMAGESTACKER_COLLECTION_H
