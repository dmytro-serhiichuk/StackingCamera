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
        ColorSpace colorSpace;
        Depth depth;
        char* filePath;

        BitmapPtr(Bitmap& bitmap);
        BitmapPtr(uint32_t w, uint32_t h, void* b, ColorSpace cs, Depth d);
    };
}

#endif //STACKINGCAMERA_BITMAP_PTR_H
