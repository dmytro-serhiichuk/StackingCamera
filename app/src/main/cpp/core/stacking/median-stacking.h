//
// Created by sedv2 on 14.01.2026.
//

#ifndef STACKINGCAMERA_MEDIAN_STACKING_H
#define STACKINGCAMERA_MEDIAN_STACKING_H

#include "base-stacking.h"

using namespace ImageIO;

class MedianStacking : public BaseStacking {
private:
    uint8_t get8(uint8_t *values, size_t size) override;
    uint16_t get16(uint16_t *values, size_t size) override;
};

#endif //STACKINGCAMERA_MEDIAN_STACKING_H
