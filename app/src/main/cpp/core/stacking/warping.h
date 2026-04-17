//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_WARPING_H
#define STACKINGCAMERA_WARPING_H

#include <matching/ransac.h>
#include "core.h"

using namespace ImageIO;

class WarpManager {
public:
    cl_mem outputBuffer;
    cl_kernel kernel;
    uint32_t outputWidth;
    uint32_t outputHeight;
    size_t outputBufferLength;
    size_t outputBufferSize;

    explicit WarpManager(BitmapPtr& baseBitmap);
    ~WarpManager();
    List<BitmapPtr> *warp(List<Core::Data> &sources, uint32_t bestIndex, Buffer<RANSAC::Result> &homographies) const;

private:
    BitmapPtr *warpSingleBitmap(Bitmap &bitmap, const Eigen::Matrix3d &H) const;
};

#endif //STACKINGCAMERA_WARPING_H
