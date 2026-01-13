//
// Created by sedv2 on 12.01.2026.
//

#ifndef STACKINGCAMERA_BITMAP_OPERATIONS_H
#define STACKINGCAMERA_BITMAP_OPERATIONS_H

#include "../imageio/bitmap.h"
#include "../opencl.h"

using namespace ImageIO;

typedef struct GaussianKernel {
    constexpr static float BASE_SIGMA = 1.0f;

    float* buffer;
    int32_t radius;
    size_t sizeOf;

    GaussianKernel(float* _buffer, int32_t _radius, size_t _sizeof);
    ~GaussianKernel();

    static GaussianKernel* create(float scaleFactor);
} GaussianKernel;

typedef struct BitmapInfo {
    uint32_t width;
    uint32_t height;
    size_t bufferLength;
    uint32_t stride;
    ColorSpace colorSpace;
    Depth depth;

    BitmapInfo(uint32_t w, uint32_t h, ColorSpace cs, Depth d) : width(w), height(h), colorSpace(cs), depth(d) {
        update();
    }

    void update();
    inline size_t sizeOfBuffer() const {
        return bufferLength * (size_t)depth;
    }
} BitmapInfo;

void toGray8(BitmapInfo &bitmap, cl_mem &inputBuffer);
Bitmap* toGray8WithReading(Bitmap &bitmap, cl_mem &inputBuffer);
void CLAHE(BitmapInfo &bitmap, cl_mem &inputBuffer, uint32_t tileCount=32, float fClipLimit=4.0f);
void blur(BitmapInfo &bitmap, cl_mem &inputBuffer, GaussianKernel &gk);
bool resize(BitmapInfo &bitmap, cl_mem &inputBuffer, float scaleFactor, uint32_t minSize);
int32_t* getIntegralImage(Bitmap &bitmap);

#endif //STACKINGCAMERA_BITMAP_OPERATIONS_H
