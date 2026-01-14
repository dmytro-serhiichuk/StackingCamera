//
// Created by sedv2 on 10.01.2026.
//

#include "bitmap-ptr.h"

namespace ImageIO {
    BitmapPtr::BitmapPtr(uint32_t w, uint32_t h, void *b, ColorSpace cs, Depth d) :
        width(w), height(h), colorSpace(cs), depth(d), bufferSize(w * h * (size_t)cs * (size_t)d)
    {

        // TODO: storing bitmap's data into the file
    }

    BitmapPtr::BitmapPtr(Bitmap &bitmap) :
        width(bitmap.width), height(bitmap.height),
        colorSpace(bitmap.colorSpace), depth(bitmap.depth)
    {
        // TODO: storing bitmap's data into the file
    }

    BitmapPtr::~BitmapPtr() {
        remove(filePath);
        delete filePath;
        width = 0;
        height = 0;
        bufferSize = 0;
    }

    Bitmap *BitmapPtr::read() const {
        auto buffer = new uint8_t[bufferSize];

        FILE *file = fopen(filePath, "r");
        fread(buffer, sizeof(uint8_t), bufferSize, file);
        fclose(file);

        return new Bitmap{width,height,buffer,colorSpace,depth};
    }

    uint8_t *BitmapPtr::readChunk(size_t offset, size_t size) const {
        if (size > bufferSize) size = bufferSize;
        auto buffer = new uint8_t[size];

        FILE *file = fopen(filePath, "r");
        fseek(file, offset, SEEK_SET);
        fread(buffer, sizeof(uint8_t), size, file);
        fclose(file);

        return buffer;
    }
}