//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_BITMAP_H
#define STACKINGCAMERA_BITMAP_H

#include <lcms2.h>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include "color-model.h"
#include "color-space.h"

namespace ImageIO {
    enum class Depth {
        U8 = 1,
        U16 = 2
    };

    class Bitmap {
    public:
        uint8_t *buffer = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        size_t totalSamples = 0;
        size_t bufferSize = 0;
        size_t stride = 0;
        ColorModel colorModel = ColorModel::RGB;
        ColorSpace colorSpace = ColorSpace::Other;
        Depth depth = Depth::U8;

        Bitmap() = default;

        Bitmap(uint32_t w, uint32_t h, void *b, Depth d, ColorModel cm, ColorSpace cs) :
                width(w), height(h), buffer((uint8_t *) b), depth(d), colorModel(cm),
                colorSpace(cs) {
            size_t spp = getSamplesPerPixel(colorModel);
            totalSamples = width * height * spp;
            bufferSize = totalSamples * (size_t) depth;
            stride = width * spp * (size_t) depth;
        }

        Bitmap(const Bitmap &other);
        Bitmap(Bitmap &&other) noexcept;
        ~Bitmap();

        Bitmap &operator=(Bitmap &&other) noexcept;
        Bitmap &operator=(const Bitmap &other) noexcept;

        [[nodiscard]] Bitmap copy() const;

        // Converts an image to RGB with the specified color depth (note: input data must be RGB/RGBA)
        [[nodiscard]] Bitmap convertDepth(Depth outDepth) const;
        // Converts an image to RGB with the specified color depth and color space
        Bitmap normalize(Depth outDepth, ColorSpace outColorSpace, cmsHPROFILE inProfile) const;
    };
}


#endif //STACKINGCAMERA_BITMAP_H
