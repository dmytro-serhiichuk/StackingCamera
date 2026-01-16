//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_WARPING_H
#define STACKINGCAMERA_WARPING_H

#include <eigen3/Eigen/Dense>
#include "core.h"

using namespace ImageIO;

class WarpManager {
public:
    cl_mem outputBuffer;
    cl_kernel kernel;
    int32_t outputWidth;
    int32_t outputHeight;
    size_t outputBufferLength;
    size_t outputBufferSize;

    WarpManager(BitmapPtr& baseBitmap);
    ~WarpManager();
    List<BitmapPtr> *warp(List<Core::Data> &sources, uint32_t bestIndex, Buffer<Eigen::Matrix3d> &matrices);

private:
    BitmapPtr *warpSingleBitmap(Bitmap &bitmap, const Eigen::Matrix3d &H);
};

#endif //STACKINGCAMERA_WARPING_H
