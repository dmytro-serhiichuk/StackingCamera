//
// Created by sedv2 on 08.05.2026.
//

#ifndef STACKINGCAMERA_AVERAGE_STACKING_H
#define STACKINGCAMERA_AVERAGE_STACKING_H

#include "base-stacking.h"

class AverageStacking : public BaseStacking {
private:
    uint8_t get8(uint8_t *values, size_t size) override;
    uint16_t get16(uint16_t *values, size_t size) override;
};

#endif //STACKINGCAMERA_AVERAGE_STACKING_H
