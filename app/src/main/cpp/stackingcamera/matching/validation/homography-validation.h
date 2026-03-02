//
// Created by sedv2 on 28.02.2026.
//

#ifndef STACKINGCAMERA_HOMOGRAPHY_VALIDATION_H
#define STACKINGCAMERA_HOMOGRAPHY_VALIDATION_H

#include "imageio/bitmap-ptr.h"
#include "matching/ransac.h"
#include "validation.h"
#include "core.h"

namespace Validation {
    std::vector<HomographyValidationInfo> validateMatrices(List<Core::Data> &sources,
                                                           Buffer<RANSAC::Result> &homographies,
                                                           int32_t referenceIndex);
}

#endif //STACKINGCAMERA_HOMOGRAPHY_VALIDATION_H
