//
// Created by sedv2 on 16.03.2025.
//

#ifndef IMAGESTACKER_BITMAP_8_H
#define IMAGESTACKER_BITMAP_8_H

#include "opencl-manager.h"
#include "gaussian-kernel.h"

class Bitmap8 {
public:
    int32_t width;
    int32_t height;
    uint8_t* buffer;
    size_t bufferSize;

    Bitmap8(int32_t _width, int32_t _height, uint8_t* _buffer);
    ~Bitmap8();

    void CLAHE(cl_mem &inputBuffer, uint32_t tileCount=32, float fClipLimit=4.0f);
    void blur(cl_mem &inputBuffer, GaussianKernel &gk);
    bool canResize(float scaleFactor, uint32_t minSize);
    void resize(cl_mem &inputBuffer, float scaleFactor);
    int32_t* integral();
};

#endif //IMAGESTACKER_BITMAP_8_H
