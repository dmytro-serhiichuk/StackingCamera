//
// Created by sedv2 on 05.01.2025.
//

#ifndef IMAGESTACKER_IMAGEIO_H
#define IMAGESTACKER_IMAGEIO_H

#include "bitmap-pointer.h"

namespace ImageIO {
    BitmapPointer *openImage(int fd);
}

#endif //IMAGESTACKER_IMAGEIO_H