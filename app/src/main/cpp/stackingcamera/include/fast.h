//
// Created by sedv2 on 15.01.2025.
//

#ifndef IMAGESTACKER_FAST_H
#define IMAGESTACKER_FAST_H

#include "brisk.h"

namespace FAST {
    void detect(Bitmap8 &bitmap, cl_mem &imBuffer, Buffer<KeyPoint> &keyPoints, cl_mem &kpsBuffer,
                cl_mem &kpsCounterBuffer, uint32_t padding, uint32_t octave, float scaleFactor);
}

#endif //IMAGESTACKER_FAST_H
