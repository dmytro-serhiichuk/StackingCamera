//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_JPEG_IO_H
#define STACKINGCAMERA_JPEG_IO_H

#include "imageio.h"

namespace ImageIO {
    BitmapPtr* loadJPEG(const uint8_t* fileData, size_t fileSize, ColorSpace colorSpace, Depth depth);

    void saveJPEG(int fd, Bitmap &bmp, SaveProperties props);
}

#endif //STACKINGCAMERA_JPEG_IO_H
