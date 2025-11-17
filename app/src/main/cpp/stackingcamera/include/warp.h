//
// Created by sedv2 on 17.01.2025.
//

#ifndef IMAGESTACKER_WARP_H
#define IMAGESTACKER_WARP_H

#include "bitmap-pointer.h"
#include <eigen3/Eigen/Dense>

class Warper {
public:
    cl_mem outputBuffer;
    cl_kernel kernel;
    int32_t outputWidth;
    int32_t outputHeight;
    size_t outputBufferSize;

    Warper(Bitmap* bitmap);

    ~Warper();

    BitmapPointer *warpPerspective(Bitmap *bitmap, const Eigen::Matrix3d &H);
};

#endif //IMAGESTACKER_WARP_H
