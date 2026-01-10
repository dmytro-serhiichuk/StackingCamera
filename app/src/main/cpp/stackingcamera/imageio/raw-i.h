//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_RAW_I_H
#define STACKINGCAMERA_RAW_I_H

#include "imageio.h"

namespace ImageIO {
    bool isRAW(void* buffer, size_t size);

    BitmapPtr* loadRAW(uint8_t* fileData, size_t fileSize, ColorSpace colorSpace, Depth depth);
}

#endif //STACKINGCAMERA_RAW_I_H
