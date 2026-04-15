//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_PNG_IO_H
#define STACKINGCAMERA_PNG_IO_H

#include "imageio.h"

namespace ImageIO {
    BitmapPtr* loadPNG(const uint8_t* fileData, size_t fileSize, ColorSpace colorSpace, Depth depth);

    void savePNG(int fd, Bitmap &bmp, SaveProperties props);
}

#endif //STACKINGCAMERA_PNG_IO_H
