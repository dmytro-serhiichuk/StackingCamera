//
// Created by sedv2 on 05.02.2025.
//

#ifndef IMAGESTACKER_JPEGIO_H
#define IMAGESTACKER_JPEGIO_H


#include <stdint.h>
#include "bitmap-pointer.h"

namespace ImageIO {
    BitmapPointer* openJPEG(uint8_t *jpegData, uint64_t length);

    void saveJPEG(int fd, Bitmap *bmp, int quality);
}

#endif //IMAGESTACKER_JPEGIO_H
