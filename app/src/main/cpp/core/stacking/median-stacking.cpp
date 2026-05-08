//
// Created by sedv2 on 14.01.2026.
//

#include "median-stacking.h"
#include <algorithm>

uint16_t MedianStacking::get16(uint16_t *values, size_t size) {
    std::sort(values, values + size, [](const uint16_t &a, const uint16_t &b) {
        return a < b;
    });

    if (size % 2 == 0) return (values[size / 2 - 1] + values[size / 2]) / 2;
    else return values[size / 2];
}
uint8_t MedianStacking::get8(uint8_t *values, size_t size) {
    std::sort(values, values + size, [](const uint16_t &a, const uint16_t &b) {
        return a < b;
    });

    if (size % 2 == 0) return (values[size / 2 - 1] + values[size / 2]) / 2;
    else return values[size / 2];
}