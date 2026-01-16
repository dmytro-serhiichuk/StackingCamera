//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_RANSAC_H
#define STACKINGCAMERA_RANSAC_H

#include <eigen3/Eigen/Dense>
#include "matching.h"
#include "collections/buffer.h"

namespace RANSAC {
    Buffer<Eigen::Matrix3d> *computeHomographyMatrices(List<Core::Data> &sources, uint32_t bestIndex,
                                                       Buffer<Buffer<Matching::Match>> &matches);
}

#endif //STACKINGCAMERA_RANSAC_H
