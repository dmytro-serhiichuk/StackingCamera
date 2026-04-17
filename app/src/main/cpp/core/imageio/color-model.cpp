//
// Created by sedv2 on 02.04.2026.
//

#include "color-model.h"

uint8_t ImageIO::getSamplesPerPixel(ImageIO::ColorModel colorModel) {
    switch (colorModel) {
        case ColorModel::RGBA:
            return 4;
        case ColorModel::RGB:
        case ColorModel::XYZ:
            return 3;
        default:
            return 1;
    }
}
