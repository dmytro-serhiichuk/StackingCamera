//
// Created by sedv2 on 10.01.2026.
//

#ifndef STACKINGCAMERA_IMAGEIO_H
#define STACKINGCAMERA_IMAGEIO_H

#include "bitmap-ptr.h"

namespace ImageIO {
    typedef struct SaveProperties {
        int jpegQuality = 100;
    } SaveProperties;

    BitmapPtr* open(int fd, ColorSpace colorSpace, Depth depth);
    void save(int fd, Bitmap& bitmap, SaveProperties = {});
}

#endif //STACKINGCAMERA_IMAGEIO_H
