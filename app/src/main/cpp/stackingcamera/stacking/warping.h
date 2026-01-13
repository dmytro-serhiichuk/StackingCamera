//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_WARPING_H
#define STACKINGCAMERA_WARPING_H

#include <eigen3/Eigen/Dense>
#include "../imageio/bitmap-ptr.h"
#include "../opencl.h"

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
    BitmapPtr *warpPerspective(Bitmap &bitmap, const Eigen::Matrix3d &H);
};

#endif //STACKINGCAMERA_WARPING_H
