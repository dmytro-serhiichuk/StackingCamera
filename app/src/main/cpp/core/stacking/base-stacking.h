//
// Created by sedv2 on 08.05.2026.
//

#ifndef STACKINGCAMERA_BASE_STACKING_H
#define STACKINGCAMERA_BASE_STACKING_H

#include "imageio/bitmap-ptr.h"
#include "collections/list.h"

using namespace ImageIO;

class BaseStacking {
public:
    BaseStacking() = default;
    virtual ~BaseStacking() = default;
    virtual Bitmap *stack(List<BitmapPtr> &src, BitmapPtr &referenceBitmap);
private:
    virtual uint16_t get16(uint16_t *values, size_t size) = 0;
    virtual uint8_t get8(uint8_t *values, size_t size) = 0;
};

#endif //STACKINGCAMERA_BASE_STACKING_H
