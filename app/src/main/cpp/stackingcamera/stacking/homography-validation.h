//
// Created by sedv2 on 28.02.2026.
//

#ifndef STACKINGCAMERA_HOMOGRAPHY_VALIDATION_H
#define STACKINGCAMERA_HOMOGRAPHY_VALIDATION_H

#include "imageio/bitmap-ptr.h"
#include <eigen3/Eigen/Dense>

namespace HomographyValidation {
    enum class Status {
        OK = 0,
        WARNING = 1,
        BAD = 2
    };

    typedef struct ValidationInfo {
        static const int32_t FIELDS_NUMBER = 9;

        Status scale;
        Status translationX;
        Status translationY;
        Status perspective;
        Status shear;
        Status anisotropy;
        bool isConvex;
        bool mirrored;
        int32_t bitmapIndex;
    } ValidationInfo;

    ValidationInfo validate(Eigen::Matrix3d &matrix, int32_t width, int32_t height);
}

#endif //STACKINGCAMERA_HOMOGRAPHY_VALIDATION_H
