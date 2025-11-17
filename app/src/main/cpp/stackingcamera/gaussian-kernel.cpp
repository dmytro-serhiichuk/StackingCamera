//
// Created by sedv2 on 16.03.2025.
//

#include <cmath>
#include "gaussian-kernel.h"

GaussianKernel::GaussianKernel(float *_buffer, int32_t _radius, size_t _sizeof) {
    buffer = _buffer;
    radius = _radius;
    sizeOf = _sizeof;
}

GaussianKernel::~GaussianKernel() {
    delete [] buffer;
}

GaussianKernel* GaussianKernel::createKernel(float scaleFactor) {
    float sigma = BASE_SIGMA * scaleFactor;

    int32_t radius = (int32_t)ceil(3 * sigma);
    int32_t diameter = 2 * radius + 1;
    float* buffer = new float[diameter * diameter];

    float sum = 0.0f;

    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            float value = exp(-(x * x + y * y) / (2 * sigma * sigma));
            buffer[(y + radius) * diameter + (x + radius)] = value;
            sum += value;
        }
    }
    for (size_t i = 0; i < diameter * diameter; i++) {
        buffer[i] /= sum;
    }
    return new GaussianKernel(buffer, radius, diameter * diameter * sizeof(float));
}