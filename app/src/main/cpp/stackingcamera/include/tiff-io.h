//
// Created by sedv2 on 09.01.2025.
//

#ifndef IMAGESTACKER_TIFF_IO_H
#define IMAGESTACKER_TIFF_IO_H

#include <cstdint>

#include "imageio.h"

namespace ImageIO {
    BitmapPointer* openTIFF(uint8_t *tiffData, uint64_t length);

    void saveTIFF(int fd, Bitmap *bmp);
}

#endif //IMAGESTACKER_TIFF_IO_H
