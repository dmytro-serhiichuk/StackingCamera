//
// Created by sedv2 on 10.01.2026.
//

#include "png-io.h"
#include <png.h>
#include <stdexcept>
#include <cmath>

namespace ImageIO {
    BitmapPtr *loadPNG(uint8_t *fileData, size_t fileSize, ColorSpace colorSpace, Depth depth) {
        png_image image;
        memset(&image, 0, sizeof(image));
        image.version = PNG_IMAGE_VERSION;

        if (!png_image_begin_read_from_memory(&image, fileData, fileSize)) {
            png_image_free(&image);
            throw std::runtime_error("Failed to begin read file");
        }

        image.format = 0;
        if (colorSpace == ColorSpace::RGB || colorSpace == ColorSpace::RGBA) {
            image.format |= PNG_FORMAT_FLAG_COLOR;
        }
        if (colorSpace == ColorSpace::RGBA) {
            image.format |= PNG_FORMAT_FLAG_ALPHA;
        }
        if (depth != Depth::U8) {
            image.format |= PNG_FORMAT_FLAG_LINEAR;
        }

        size_t bufferSize = PNG_IMAGE_SIZE(image);
        uint8_t* buffer = new uint8_t[bufferSize];

        if (!png_image_finish_read(&image, nullptr, buffer, 0, nullptr)) {
            png_image_free(&image);
            delete [] buffer;
            throw std::runtime_error("Failed to finish read file");
        }

        if (depth != Depth::U8) {
            double gamma = 1.0 / 2.2;
            uint16_t* buffer16 = (uint16_t*)buffer;
            const size_t len = bufferSize / 2;
            for (size_t i = 0; i < len; i++) {
                buffer16[i] = std::pow(buffer16[i] / 65535.0, gamma) * 65535.0;
            }
        }

        Bitmap* bmp = new Bitmap(image.width, image.height, buffer, colorSpace, depth);
        BitmapPtr* bitmapPtr = new BitmapPtr(*bmp);
        delete bmp;

        png_image_free(&image);

        return bitmapPtr;
    }
}
