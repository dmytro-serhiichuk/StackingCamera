//
// Created by sedv2 on 15.01.2025.
//

#ifndef IMAGESTACKER_BITMAP_H
#define IMAGESTACKER_BITMAP_H

#include "bitmap-8.h"

enum class ColorType {
    RGB,
    Grayscale
};

class Bitmap {
public:
    int32_t width;
    int32_t height;
    uint16_t* buffer;
    uint64_t bufferLength;
    ColorType colorType;

    Bitmap(int32_t w, int32_t h, uint16_t* b, ColorType ct = ColorType::RGB);

    ~Bitmap();

    cl_mem createCLBuffer();
    Bitmap8* toGray8(cl_mem &inputBuffer);
    Bitmap8* toGray8WithBufferReading(cl_mem &inputBuffer);
};

#endif //IMAGESTACKER_BITMAP_H
