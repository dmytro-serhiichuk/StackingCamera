//
// Created by sedv2 on 10.01.2025.
//

#ifndef IMAGESTACKER_PNGIO_H
#define IMAGESTACKER_PNGIO_H

#include <cstdint>

#include "imageio.h"

namespace ImageIO {
    BitmapPointer* openPNG(uint8_t *pngData, uint64_t length);

    void savePNG(int fd, Bitmap* bitmap);
}

#endif //IMAGESTACKER_PNGIO_H
