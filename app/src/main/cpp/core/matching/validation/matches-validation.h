//
// Created by sedv2 on 02.03.2026.
//

#ifndef STACKINGCAMERA_MATCHES_VALIDATION_H
#define STACKINGCAMERA_MATCHES_VALIDATION_H

#include "validation.h"
#include "core.h"
#include "matching/ransac.h"

namespace Validation {
    std::vector<MatchesValidationInfo> validateMatches(List<Core::Data> &sources,
                                                       Buffer<Buffer<Matching::Match>> &matches,
                                                       Buffer<RANSAC::Result> &ransacResults,
                                                       int32_t referenceIndex);
}

#endif //STACKINGCAMERA_MATCHES_VALIDATION_H
