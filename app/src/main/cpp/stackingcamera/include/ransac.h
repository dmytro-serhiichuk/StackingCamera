//
// Created by sedv2 on 17.01.2025.
//

#ifndef IMAGESTACKER_RANSAC_H
#define IMAGESTACKER_RANSAC_H

#include <eigen3/Eigen/Dense>
#include "matcher.h"
#include "buffer.h"

namespace RANSAC {
    Eigen::Matrix3d computeHomography(
            Buffer<Match> &matches,
            Buffer<KeyPoint> &kps1,
            Buffer<KeyPoint> &kps2
    );
}

#endif //IMAGESTACKER_RANSAC_H
