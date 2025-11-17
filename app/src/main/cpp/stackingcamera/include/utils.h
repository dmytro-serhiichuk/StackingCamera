//
// Created by sedv2 on 05.04.2025.
//

#ifndef IMAGESTACKER_UTILS_H
#define IMAGESTACKER_UTILS_H

#include "core.h"
#include "matcher.h"

namespace Utils {
    void drawKeyPoints(Bitmap &bmp, Buffer<KeyPoint> &kps);

    void drawAllMatches(Buffer<Buffer<Match>> &matches, uint32_t bestIndex);
}

#endif //IMAGESTACKER_UTILS_H
