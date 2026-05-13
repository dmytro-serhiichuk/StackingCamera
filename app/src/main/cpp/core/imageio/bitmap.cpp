//
// Created by sedv2 on 10.01.2026.
//

#include "bitmap.h"
#include <stdexcept>
#include "profiles-manager.h"

namespace ImageIO {
    Bitmap::Bitmap(const Bitmap &other) {
        width = other.width;
        height = other.height;
        totalSamples = other.totalSamples;
        bufferSize = other.bufferSize;
        stride = other.stride;
        colorModel = other.colorModel;
        colorSpace = other.colorSpace;
        depth = other.depth;

        buffer = new uint8_t[bufferSize];
        memcpy(buffer, other.buffer, bufferSize);
    }
    Bitmap::Bitmap(Bitmap &&other) noexcept {
        width = other.width;
        height = other.height;
        totalSamples = other.totalSamples;
        bufferSize = other.bufferSize;
        stride = other.stride;
        colorModel = other.colorModel;
        colorSpace = other.colorSpace;
        depth = other.depth;

        buffer = other.buffer;
        other.buffer = nullptr;
    }
    Bitmap::~Bitmap() {
        delete [] buffer;
        buffer = nullptr;
    }

    Bitmap &Bitmap::operator=(Bitmap &&other) noexcept {
        if (this != &other) {
            delete [] buffer;

            width = other.width;
            height = other.height;
            totalSamples = other.totalSamples;
            bufferSize = other.bufferSize;
            stride = other.stride;
            colorModel = other.colorModel;
            colorSpace = other.colorSpace;
            depth = other.depth;

            buffer = other.buffer;
            other.buffer = nullptr;
        }
        return *this;
    }
    Bitmap &Bitmap::operator=(const Bitmap &other) noexcept {
        if (this != &other) {
            delete [] buffer;

            width = other.width;
            height = other.height;
            totalSamples = other.totalSamples;
            bufferSize = other.bufferSize;
            stride = other.stride;
            colorModel = other.colorModel;
            colorSpace = other.colorSpace;
            depth = other.depth;

            buffer = new uint8_t[bufferSize];
            memcpy(buffer, other.buffer, bufferSize);
        }
        return *this;
    }

    [[nodiscard]] Bitmap Bitmap::copy() const {
        auto cb = new uint8_t[bufferSize];
        memcpy(cb, buffer, bufferSize);
        return Bitmap {width, height, cb, depth, colorModel, colorSpace };
    }

    Bitmap Bitmap::convertToRgbWithDepth(Depth outDepth) const {
        if (depth == outDepth && colorModel == ColorModel::RGB) return copy();

        size_t outTotalSamples = width * height * getSamplesPerPixel(ColorModel::RGB);
        auto outBuffer = new uint8_t [outTotalSamples * (size_t)outDepth];

        if (colorModel == ColorModel::RGBA) {
            if (outDepth == Depth::U8) {
                auto srcBuffer = (uint16_t*)buffer;
                for (size_t inI = 0, outI = 0; inI < totalSamples; inI += 4, outI += 3) {
                    outBuffer[outI]     = (uint8_t)(srcBuffer[inI]     >> 8);
                    outBuffer[outI + 1] = (uint8_t)(srcBuffer[inI + 1] >> 8);
                    outBuffer[outI + 2] = (uint8_t)(srcBuffer[inI + 2] >> 8);
                }
            } else {
                for (size_t inI = 0, outI = 0; inI < totalSamples; inI += 4, outI += 3) {
                    ((uint16_t*)outBuffer)[outI]     = (uint16_t)(buffer[inI]     * 257);
                    ((uint16_t*)outBuffer)[outI + 1] = (uint16_t)(buffer[inI + 1] * 257);
                    ((uint16_t*)outBuffer)[outI + 2] = (uint16_t)(buffer[inI + 2] * 257);
                }
            }
        } else {
            if (outDepth == Depth::U8) {
                auto srcBuffer = (uint16_t*)buffer;
                for (size_t i = 0; i < totalSamples; i++) {
                    outBuffer[i] = (uint8_t)(srcBuffer[i] >> 8);
                }
            } else {
                for (size_t i = 0; i < totalSamples; i++) {
                    ((uint16_t*)outBuffer)[i] = (uint16_t)(buffer[i] * 257);
                }
            }
        }

        return Bitmap {width, height, outBuffer, outDepth, ColorModel::RGB, colorSpace};
    }

    Bitmap Bitmap::convert(Depth outDepth, ColorSpace outColorSpace, cmsHPROFILE inProfile) const {
        const ColorModel outColorModel = ColorModel::RGB;

        if (depth == outDepth && colorSpace == outColorSpace && colorModel == outColorModel) return copy();

        const auto inType  = buildLcmsType(colorModel, depth);
        const auto outType = buildLcmsType(outColorModel, outDepth);

        auto outProfile = createProfileFromColorSpace(outColorSpace);

        cmsHTRANSFORM t = cmsCreateTransform(
            inProfile, inType,
            outProfile, outType,
            INTENT_RELATIVE_COLORIMETRIC, 0
        );

        size_t size = width * height;
        size_t outBufferSize = size * getSamplesPerPixel(outColorModel) * (size_t)outDepth;
        auto outBuffer = new uint8_t [outBufferSize];;
        cmsDoTransform(t, buffer, outBuffer, size);

        cmsDeleteTransform(t);
        cmsCloseProfile(outProfile);

        return Bitmap {width, height, outBuffer, outDepth, outColorModel, outColorSpace};
    }
}