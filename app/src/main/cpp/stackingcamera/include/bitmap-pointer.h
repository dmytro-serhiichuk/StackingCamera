//
// Created by sedv2 on 26.01.2025.
//

#ifndef IMAGESTACKER_BITMAPPOINTER_H
#define IMAGESTACKER_BITMAPPOINTER_H

#include "bitmap.h"

class BitmapPointer {
public:
    int32_t width;
    int32_t height;
    uint64_t bufferLength;
    ColorType colorType;
    char* filePath;

    BitmapPointer(int32_t w, int32_t h, uint16_t* b, ColorType ct = ColorType::RGB);

    ~BitmapPointer();

    // TODO: add checking
    Bitmap *read();

    uint16_t* readChunk(size_t offset, size_t count);
};

#endif //IMAGESTACKER_BITMAPPOINTER_H
