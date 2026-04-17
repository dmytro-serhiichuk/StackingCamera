//
// Created by sedv2 on 02.04.2026.
//

#ifndef STACKINGCAMERA_COLOR_MODEL_H
#define STACKINGCAMERA_COLOR_MODEL_H

#include <cstdint>

namespace ImageIO {
    enum class ColorModel : uint32_t {
        RGB,
        RGBA,
        GRAY,
        XYZ,
    };

    uint8_t getSamplesPerPixel(ColorModel colorModel);
}

#endif //STACKINGCAMERA_COLOR_MODEL_H
