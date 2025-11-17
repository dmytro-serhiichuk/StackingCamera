//
// Created by sedv2 on 06.02.2025.
//

#ifndef IMAGESTACKER_MATCHER_H
#define IMAGESTACKER_MATCHER_H

#include "opencl-manager.h"
#include "buffer.h"
#include "brisk.h"
#include "list.h"
#include "bitmap-pointer.h"

typedef struct Match {
    uint32_t index1;
    uint32_t index2; // key descriptor
    uint32_t distance;

    Match();
    Match(uint32_t _i1, uint32_t _i2, uint32_t _dist);
} Match;

namespace Matcher {
    Buffer<Buffer<Match>>* match(BitmapPointer &bmp, List<Descriptors> &descriptors, Buffer<KeyPoint> &keyPoints, uint32_t bestIndex);
}

#endif //IMAGESTACKER_MATCHER_H
