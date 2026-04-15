//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_BITMAP_PTR_H
#define STACKINGCAMERA_BITMAP_PTR_H

#include "bitmap.h"

namespace ImageIO {
    class BitmapPtr {
    public:
        uint32_t width;
        uint32_t height;
        Depth depth;
        ColorModel colorModel;
        ColorSpace colorSpace;
        uint32_t bufferSize;
        char* filePath;

        explicit BitmapPtr(Bitmap& bitmap);
        BitmapPtr(uint32_t w, uint32_t h, void* b, Depth d, ColorModel cm, ColorSpace cs);
        ~BitmapPtr();

        [[nodiscard]] Bitmap read() const;
        [[nodiscard]] uint8_t* readChunk(size_t offset, size_t size) const;
    };
}

#endif //STACKINGCAMERA_BITMAP_PTR_H
