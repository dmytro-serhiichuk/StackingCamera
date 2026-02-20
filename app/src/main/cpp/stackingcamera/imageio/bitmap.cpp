//
// Created by sedv2 on 10.01.2026.
//

#include "bitmap.h"
#include <stdexcept>

namespace ImageIO {
    Bitmap::~Bitmap() {
        delete [] buffer;
        buffer = nullptr;
    }

    Bitmap *Bitmap::copy() const {
        size_t size = bufferLength * (size_t)depth;
        uint8_t* cb = new uint8_t[size];
        memcpy(cb, buffer, size);
        return new Bitmap(width, height, cb, colorSpace, depth);
    }

    Bitmap *Bitmap::convertTo(Depth newDepth, ColorSpace newColorSpace) const {
        if (newDepth == depth && newColorSpace == colorSpace) {
            return copy();
        }

        if (depth == Depth::U8) {
            if      (newDepth == Depth::U8)  return convert<uint8_t, uint8_t>(newDepth, newColorSpace);
            else if (newDepth == Depth::U16) return convert<uint8_t, uint16_t>(newDepth, newColorSpace);
        } else if (depth == Depth::U16) {
            if      (newDepth == Depth::U8)  return convert<uint16_t, uint8_t>(newDepth, newColorSpace);
            else if (newDepth == Depth::U16) return convert<uint16_t, uint16_t>(newDepth, newColorSpace);
        }

        throw std::runtime_error("Unsupported convert arguments");
    }
}