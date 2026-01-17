//
// Created by sedv2 on 17.01.2026.
//

#ifndef STACKINGCAMERA_UTILS_H
#define STACKINGCAMERA_UTILS_H

#include "core.h"
#include "stacking/matching.h"

using namespace ImageIO;

namespace Utils {
    void drawKeyPoints(Bitmap &bmp, Buffer<KeyPoint> &kps);
    void drawAllMatches(Buffer<Buffer<Matching::Match>> &matches, uint32_t bestIndex);
}

#endif //STACKINGCAMERA_UTILS_H
