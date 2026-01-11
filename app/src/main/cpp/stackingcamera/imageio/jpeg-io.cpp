//
// Created by sedv2 on 10.01.2026.
//

#include "jpeg-io.h"
#include <turbojpeg.h>
#include <stdexcept>
#include <unistd.h>

namespace ImageIO {
    BitmapPtr *loadJPEG(uint8_t *fileData, size_t fileSize, ColorSpace colorSpace, Depth depth) {
        tjhandle decompressor = tjInitDecompress();
        if (decompressor == nullptr) {
            throw std::runtime_error("Failed to init jpeg decompressor");
        }

        int32_t width, height, jpegSubsamp;
        if (tjDecompressHeader2(decompressor, fileData, fileSize, &width, &height, &jpegSubsamp) < 0) {
            tjDestroy(decompressor);
            throw std::runtime_error("Failed to decompress jpeg header");
        }

        size_t bufferLength = width * height * (size_t)colorSpace;
        uint8_t* buffer = new uint8_t[bufferLength];

        int pixelFormat = colorSpace == ColorSpace::RGB ? TJPF_RGB :
                          colorSpace == ColorSpace::RGBA ? TJPF_RGBA :
                          TJPF_GRAY;

        if (tjDecompress2(decompressor, fileData, fileSize, buffer, width, 0, height, pixelFormat, TJFLAG_FASTDCT) < 0) {
            tjDestroy(decompressor);
            delete[] buffer;
            throw std::runtime_error("Failed to decompress jpeg");
        }

        tjDestroy(decompressor);

        Bitmap* bmp = new Bitmap(width, height, buffer, colorSpace, Depth::U8);
        if (depth != Depth::U8) {
            Bitmap* temp = bmp;
            bmp = temp->convertDepth(Depth::U16);
            delete temp;
        }

        BitmapPtr* bitmapPtr = new BitmapPtr(*bmp);
        delete bmp;
        return bitmapPtr;
    }

    void saveJPEG(int fd, Bitmap &bmp, SaveProperties props) {
        Bitmap* bp = &bmp;
        if (bp->depth != Depth::U8) {
            bp = bp->convertDepth(Depth::U8);
        }

        unsigned long jpegSize = 0;
        uint8_t* jpegBuf = nullptr;

        tjhandle jpegCompressor = tjInitCompress();
        if (!jpegCompressor) {
            throw std::runtime_error("Failed to init jpeg compressor");
        }

        jpegSize = 0;
        jpegBuf = nullptr;

        int pixelFormat = bp->colorSpace == ColorSpace::RGB ? TJPF_RGB :
                          bp->colorSpace == ColorSpace::RGBA ? TJPF_RGBA :
                          TJPF_GRAY;

        int subsamp = (bp->colorSpace == ColorSpace::RGB ||
                       bp->colorSpace == ColorSpace::RGBA)
                       ? TJSAMP_444
                       : TJSAMP_GRAY;

        if (tjCompress2(
                jpegCompressor, bp->buffer, bp->width, 0, bp->height,
                pixelFormat, &jpegBuf, &jpegSize, subsamp, props.jpegQuality, TJFLAG_FASTDCT) < 0)
        {
            tjDestroy(jpegCompressor);
            throw std::runtime_error("Failed to compress jpeg");
        }

        write(fd, jpegBuf, jpegSize);

        tjDestroy(jpegCompressor);
        tjFree(jpegBuf);

        if (bp->depth != bmp.depth) {
            delete bp;
        }
    }
}

