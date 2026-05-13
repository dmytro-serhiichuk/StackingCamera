//
// Created by sedv2 on 02.04.2026.
//

#ifndef STACKINGCAMERA_COLOR_SPACE_H
#define STACKINGCAMERA_COLOR_SPACE_H

namespace ImageIO {
    enum class ColorSpace {
        sRGB = 0,
        Linear_sRGB = 1,
        AdobeRGB = 2,
        ProPhoto = 3,
        Other = 4
    };
}

#endif //STACKINGCAMERA_COLOR_SPACE_H
