//
// Created by sedv2 on 10.01.2026.
//

#include "bitmap-ptr.h"

namespace ImageIO {
    BitmapPtr::BitmapPtr(uint32_t w, uint32_t h, void *b, ColorSpace cs, Depth d) :
        width(w), height(h), colorSpace(cs), depth(d)
    {
        // TODO: storing bitmap's data into the file
    }

    BitmapPtr::BitmapPtr(Bitmap &bitmap) :
        width(bitmap.width), height(bitmap.height),
        colorSpace(bitmap.colorSpace), depth(bitmap.depth)
    {
        // TODO: storing bitmap's data into the file
    }
}