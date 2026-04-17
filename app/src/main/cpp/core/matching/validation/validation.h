//
// Created by sedv2 on 02.03.2026.
//

#ifndef STACKINGCAMERA_VALIDATION_H
#define STACKINGCAMERA_VALIDATION_H

#include <cstdint>

namespace Validation {
    enum class Status {
        OK = 0,
        WARNING = 1,
        BAD = 2
    };

    typedef struct HomographyValidationInfo {
        static const int32_t FIELDS_NUMBER = 8;

        Status scale;
        Status translationX;
        Status translationY;
        Status perspective;
        Status shear;
        Status anisotropy;
        bool isConvex;
        bool mirrored;
    } HomographyValidationInfo;

    typedef struct MatchesValidationInfo {
        static const int32_t FIELDS_NUMBER = 3;

        Status inliersPercentage;
        Status inliersNumber;
        Status evenDistribution;
    } MatchesValidationInfo;

    typedef struct ValidationInfo {
        static const int32_t FIELDS_NUMBER = HomographyValidationInfo::FIELDS_NUMBER + MatchesValidationInfo::FIELDS_NUMBER + 1;

        HomographyValidationInfo homographyValidationInfo;
        MatchesValidationInfo matchesValidationInfo;
        int32_t bitmapIndex;
    } ValidationInfo;
}

#endif //STACKINGCAMERA_VALIDATION_H
