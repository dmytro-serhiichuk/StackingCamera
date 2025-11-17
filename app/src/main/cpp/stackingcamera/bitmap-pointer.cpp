//
// Created by sedv2 on 26.01.2025.
//

#include <stdio.h>
#include "bitmap-pointer.h"
#include "core.h"

BitmapPointer::BitmapPointer(int32_t w, int32_t h, uint16_t *b, ColorType ct) {
    width = w;
    height = h;
    colorType = ct;

    bufferLength = colorType == ColorType::RGB ? w * h * 3 : w * h;

    filePath = Core::jniHelper->createTempFile();

    FILE *file = fopen(filePath, "w");
    fwrite(b, sizeof(uint16_t), bufferLength, file);
    fclose(file);

    delete [] b;
}

BitmapPointer::~BitmapPointer() {
    remove(filePath);
    delete filePath;
    width = 0;
    height = 0;
    bufferLength = 0;
    colorType = ColorType::Grayscale;
}

Bitmap *BitmapPointer::read() {
    FILE *file = fopen(filePath, "r");
    uint16_t *buffer = new uint16_t[bufferLength];
    fread(buffer, sizeof(uint16_t), bufferLength, file);
    fclose(file);

    return new Bitmap{
            width,
            height,
            buffer,
            colorType
    };
}

uint16_t *BitmapPointer::readChunk(size_t offset, size_t count) {
    FILE *file = fopen(filePath, "r");
    if (count > bufferLength) {
        count = bufferLength;
    }
    uint16_t *buffer = new uint16_t[count];
    fseek(file, offset * sizeof(uint16_t), SEEK_SET);
    fread(buffer, sizeof(uint16_t), count, file);
    fclose(file);

    return buffer;
}
