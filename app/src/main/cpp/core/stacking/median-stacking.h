//
// Created by sedv2 on 14.01.2026.
//

#ifndef STACKINGCAMERA_MEDIAN_STACKING_H
#define STACKINGCAMERA_MEDIAN_STACKING_H

#include "imageio/imageio.h"
#include "collections/list.h"

using namespace ImageIO;

namespace MedianStacking {
    Bitmap *stack(List<BitmapPtr> &src, BitmapPtr &referenceBitmap);
}

#endif //STACKINGCAMERA_MEDIAN_STACKING_H
