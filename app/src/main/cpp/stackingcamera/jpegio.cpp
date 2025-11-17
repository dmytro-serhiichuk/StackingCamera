//
// Created by sedv2 on 05.02.2025.
//

#include "jpegio.h"

#include <unistd.h>
#include <turbojpeg.h>

BitmapPointer* ImageIO::openJPEG(uint8_t *jpegData, uint64_t length) {
    tjhandle decompressor = tjInitDecompress();
    if (decompressor == nullptr) {
        return nullptr;
    }

    int width, height, jpegSubsamp;
    if (tjDecompressHeader2(decompressor, jpegData, length, &width, &height, &jpegSubsamp) < 0) {
        tjDestroy(decompressor);
        return nullptr;
    }

    uint64_t bufferSize = width * height * 3;
    uint8_t* imageBuffer = new uint8_t[bufferSize];

    if (tjDecompress2(decompressor, jpegData, length, imageBuffer, width, 0, height, TJPF_RGB, TJFLAG_FASTDCT) < 0) {
        tjDestroy(decompressor);
        return nullptr;
    }

    tjDestroy(decompressor);

    uint16_t* imageBuffer_16 = new uint16_t[bufferSize];
    for (size_t i = 0; i < bufferSize; i++) {
        imageBuffer_16[i] = static_cast<uint16_t>(imageBuffer[i]) * 257;
    }

    delete [] imageBuffer;
    imageBuffer = nullptr;

    return new BitmapPointer {
            width,
            height,
            imageBuffer_16
    };
}

uint8_t *convertTo8Bit(Bitmap *bmp) {
    uint8_t *buffer = new uint8_t[bmp->bufferLength];
    for (size_t i = 0; i < bmp->bufferLength; i++) {
        buffer[i] = bmp->buffer[i] >> 8;
    }
    return buffer;
}


void ImageIO::saveJPEG(int fd, Bitmap *bmp, int quality) {
    uint8_t *buffer8 = convertTo8Bit(bmp);

    tjhandle jpegCompressor = tjInitCompress();
    if (!jpegCompressor) {
        delete [] buffer8;
        return;
    }

    unsigned long jpegSize = 0;
    uint8_t* jpegBuf = nullptr;

    int pixelFormat = (bmp->colorType == ColorType::RGB) ? TJPF_RGB : TJPF_GRAY;
    int subsamp = (bmp->colorType == ColorType::RGB) ? TJSAMP_444 : TJSAMP_GRAY;

    if (tjCompress2(jpegCompressor, buffer8, bmp->width, 0, bmp->height,
                    pixelFormat, &jpegBuf, &jpegSize, subsamp, quality, TJFLAG_FASTDCT) < 0) {
        tjDestroy(jpegCompressor);
        delete [] buffer8;
        return;
    }

    write(fd, jpegBuf, jpegSize * sizeof(uint8_t ));

    tjFree(jpegBuf);
    tjDestroy(jpegCompressor);
    delete [] buffer8;
}
