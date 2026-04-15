//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_TIFF_IO_H
#define STACKINGCAMERA_TIFF_IO_H

#include "imageio.h"

namespace ImageIO {
    BitmapPtr* loadTIFF(const uint8_t* fileData, size_t fileSize, ColorSpace colorSpace, Depth depth);

    void saveTIFF(int fd, Bitmap &bmp, SaveProperties props);
}

#endif //STACKINGCAMERA_TIFF_IO_H
