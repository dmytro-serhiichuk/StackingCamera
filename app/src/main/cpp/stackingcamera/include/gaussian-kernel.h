//
// Created by sedv2 on 16.03.2025.
//

#ifndef IMAGESTACKER_GAUSSIAN_KERNEL_H
#define IMAGESTACKER_GAUSSIAN_KERNEL_H

#include <cstdint>

typedef struct GaussianKernel {
    constexpr static float BASE_SIGMA = 1.0f;

    float* buffer;
    int32_t radius;
    size_t sizeOf;

    GaussianKernel(float* _buffer, int32_t _radius, size_t _sizeof);
    ~GaussianKernel();

    static GaussianKernel* createKernel(float scaleFactor);
} GaussianKernel;

#endif //IMAGESTACKER_GAUSSIAN_KERNEL_H
