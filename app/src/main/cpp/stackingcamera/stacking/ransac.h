//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_RANSAC_H
#define STACKINGCAMERA_RANSAC_H

#include <eigen3/Eigen/Dense>
#include "matching.h"
#include "../collections/buffer.h"

namespace RANSAC {
    Eigen::Matrix3d computeHomography(
            Buffer<Matching::Match> &matches,
            Buffer<KeyPoint> &kps1,
            Buffer<KeyPoint> &kps2
    );
}

#endif //STACKINGCAMERA_RANSAC_H
