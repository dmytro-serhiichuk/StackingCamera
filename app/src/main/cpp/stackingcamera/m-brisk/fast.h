//
// Created by sedv2 on 12.01.2026.
//

#ifndef STACKINGCAMERA_FAST_H
#define STACKINGCAMERA_FAST_H

#include "m-brisk.h"
#include "bitmap-operations.h"

namespace FAST {
    typedef struct FAST_Buffers {
        cl_mem kpsBuffer;
        cl_mem counterBuffer;

        FAST_Buffers(size_t size);
        ~FAST_Buffers();
    } FAST_Buffers;

    void detect(BitmapInfo &bitmap, cl_mem &imageBuffer, Buffer<KeyPoint> &keyPoints, FAST_Buffers &fastBuffers, uint32_t padding, uint32_t octave, float scaleFactor);
}

#endif //STACKINGCAMERA_FAST_H
