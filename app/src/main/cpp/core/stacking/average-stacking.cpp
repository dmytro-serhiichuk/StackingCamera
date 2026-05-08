//
// Created by sedv2 on 08.05.2026.
//

#include "average-stacking.h"

uint16_t AverageStacking::get16(uint16_t *values, size_t size) {
    uint64_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += values[i];
    }
    return static_cast<uint16_t>(sum / size);
}

uint8_t AverageStacking::get8(uint8_t *values, size_t size) {
    uint64_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += values[i];
    }
    return static_cast<uint8_t>(sum / size);
}
