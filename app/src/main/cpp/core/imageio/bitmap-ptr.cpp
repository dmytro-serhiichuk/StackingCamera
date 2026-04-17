//
// Created by sedv2 on 10.01.2026.
//

#include "bitmap-ptr.h"
#include "core.h"

namespace ImageIO {
    BitmapPtr::BitmapPtr(uint32_t w, uint32_t h, void* b, Depth d, ColorModel cm, ColorSpace cs) :
        width(w), height(h), depth(d), colorModel(cm), colorSpace(cs)
    {
        bufferSize = width * height * (size_t)depth * getSamplesPerPixel(colorModel);
        filePath = Core::getFileStorage()->createTempFile();

        FILE *file = fopen(filePath, "w");
        fwrite(b, sizeof(uint8_t), bufferSize, file);
        fclose(file);

        delete [] (uint8_t*)b;
    }

    BitmapPtr::BitmapPtr(Bitmap &bitmap) :
        width(bitmap.width), height(bitmap.height), colorModel(bitmap.colorModel),
        colorSpace(bitmap.colorSpace), depth(bitmap.depth), bufferSize(bitmap.bufferSize)
    {
        filePath = Core::getFileStorage()->createTempFile();

        FILE *file = fopen(filePath, "w");
        fwrite(bitmap.buffer, sizeof(uint8_t), bufferSize, file);
        fclose(file);
    }

    BitmapPtr::~BitmapPtr() {
        remove(filePath);
        delete filePath;
        width = 0;
        height = 0;
        bufferSize = 0;
    }

    Bitmap BitmapPtr::read() const {
        auto buffer = new uint8_t[bufferSize];

        FILE *file = fopen(filePath, "r");
        fread(buffer, sizeof(uint8_t), bufferSize, file);
        fclose(file);

        return Bitmap {width, height, buffer, depth, colorModel, colorSpace};
    }

    uint8_t *BitmapPtr::readChunk(size_t offset, size_t size) const {
        if (size > bufferSize) size = bufferSize;
        auto buffer = new uint8_t[size];

        FILE *file = fopen(filePath, "r");
        fseek(file, (long)offset, SEEK_SET);
        fread(buffer, sizeof(uint8_t), size, file);
        fclose(file);

        return buffer;
    }
}