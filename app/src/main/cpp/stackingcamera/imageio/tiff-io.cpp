//
// Created by sedv2 on 10.01.2026.
//

#include "tiff-io.h"
#include <tiffio.h>
#include <stdexcept>

namespace ImageIO {
    // TODO: function cannot open fd
    BitmapPtr* loadTIFF(int fd, ColorSpace colorSpace, Depth depth) {
        TIFF* tiff = TIFFFdOpen(fd, "IMAGE", "r");
        if (!tiff) {
            throw std::runtime_error("Failed to open file");
        }

        uint32_t width, height;
        uint16_t bitsPerSample, samplesPerPixel, photometric;

        TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
        TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
        TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photometric);

        if (bitsPerSample != 16 && bitsPerSample != 8) {
            TIFFClose(tiff);
            throw std::runtime_error("Only 8/16 bits per sample tiff supported");
        }
        if (samplesPerPixel != 1 && samplesPerPixel != 3 && samplesPerPixel != 4) {
            TIFFClose(tiff);
            throw std::runtime_error("Only grayscale/rgb/rgba tiff supported");
        }

        tsize_t scanlineSize = TIFFScanlineSize(tiff);
        uint8_t* scanline = new uint8_t[scanlineSize]();

        size_t bytesPerSample = bitsPerSample / 8;
        uint8_t* data = new uint8_t[width * height * bytesPerSample * samplesPerPixel];

        for (uint32_t row = 0; row < height; row++) {
            if (TIFFReadScanline(tiff, scanline, row) < 0) {
                TIFFClose(tiff);
                delete [] scanline;
                delete [] data;
                throw std::runtime_error("Failed read scanline");
            }

            for (uint32_t col = 0; col < width; col++) {
                size_t dataIndex = (row * width + col) * samplesPerPixel;
                size_t scanlineIndex = col * samplesPerPixel;

                if (bytesPerSample == 1) {
                    if (samplesPerPixel == 1) {
                        data[dataIndex] = scanline[scanlineIndex];
                    }
                    else if (samplesPerPixel == 3) {
                        data[dataIndex] = scanline[scanlineIndex];
                        data[dataIndex + 1] = scanline[scanlineIndex + 1];
                        data[dataIndex + 2] = scanline[scanlineIndex + 2];
                    }
                    else if (samplesPerPixel == 4) {
                        data[dataIndex] = scanline[scanlineIndex];
                        data[dataIndex + 1] = scanline[scanlineIndex + 1];
                        data[dataIndex + 2] = scanline[scanlineIndex + 2];
                        data[dataIndex + 3] = scanline[scanlineIndex + 3];
                    }
                }
                else {
                    uint16_t* data16 = (uint16_t*)data;
                    uint16_t* scanline16 = (uint16_t*)scanline;

                    if (samplesPerPixel == 1) {
                        data16[dataIndex] = scanline16[scanlineIndex];
                    }
                    else if (samplesPerPixel == 3) {
                        data16[dataIndex] = scanline16[scanlineIndex];
                        data16[dataIndex + 1] = scanline16[scanlineIndex + 1];
                        data16[dataIndex + 2] = scanline16[scanlineIndex + 2];
                    }
                    else if (samplesPerPixel == 4) {
                        data16[dataIndex] = scanline16[scanlineIndex];
                        data16[dataIndex + 1] = scanline16[scanlineIndex + 1];
                        data16[dataIndex + 2] = scanline16[scanlineIndex + 2];
                        data16[dataIndex + 3] = scanline16[scanlineIndex + 3];
                    }
                }
            }
        }

        TIFFClose(tiff);
        delete [] scanline;

        ColorSpace decodedColorSpace =
                samplesPerPixel == 1 ? ColorSpace::Grayscale :
                samplesPerPixel == 3 ? ColorSpace::RGB :
                ColorSpace::RGBA;

        Depth decodedDepth = bytesPerSample == 1 ? Depth::U8 : Depth::U16;

        Bitmap* bmp = new Bitmap(width, height, data, decodedColorSpace, decodedDepth);
        if (bmp->colorSpace != colorSpace || bmp->depth != depth) {
            Bitmap* tmp = bmp;
            bmp = tmp->convertTo(depth, colorSpace);
            delete tmp;
        }
        BitmapPtr* bitmapPtr = new BitmapPtr(*bmp);
        delete bmp;
        return bitmapPtr;
    }

    void saveTIFF(int fd, Bitmap &bmp, SaveProperties props) {
        TIFF* tiff = TIFFFdOpen(fd, "IMAGE", "w");
        if (!tiff) {
            throw std::runtime_error("Failed to open file");
        }

        uint16_t samplesPerPixel = bmp.colorSpace == ColorSpace::Grayscale ? 1 :
                                   bmp.colorSpace == ColorSpace::RGB ? 3 :
                                   4;

        uint16_t depthSize = (uint16_t)bmp.depth;
        uint16_t bitsPerSample = depthSize * 8;

        TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH,      bmp.width);
        TIFFSetField(tiff, TIFFTAG_IMAGELENGTH,     bmp.height);
        TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel);
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE,   bitsPerSample);
        TIFFSetField(tiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);
        TIFFSetField(tiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);

        uint16_t photometric = (samplesPerPixel == 1
                                ? PHOTOMETRIC_MINISBLACK
                                : PHOTOMETRIC_RGB);
        TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric);

        if (samplesPerPixel == 4) {
            uint16_t extraSamples = EXTRASAMPLE_ASSOCALPHA;
            TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &extraSamples);
        }

        for (uint32_t row = 0; row < bmp.height; row++) {
            if (TIFFWriteScanline(tiff, bmp.buffer + row * bmp.stride * depthSize, row, 0) < 0) {
                TIFFClose(tiff);
                throw std::runtime_error("Failed to write tiff");
            }
        }

        TIFFClose(tiff);
    }
}
