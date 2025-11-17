//
// Created by sedv2 on 09.01.2025.
//

#include <string>
#include <unistd.h>
#include "tiff-io.h"

#include <tiffio.h>

namespace ImageIO {
    typedef struct {
        const uint8_t * data;
        size_t size;
        size_t offset;
    } MemoryStream;

    tsize_t memRead(thandle_t handle, tdata_t buffer, tsize_t size) {
        MemoryStream* stream = (MemoryStream*)handle;
        if (stream->offset + size > stream->size) {
            size = stream->size - stream->offset;
        }
        memcpy(buffer, stream->data + stream->offset, size);
        stream->offset += size;
        return size;
    }

    tsize_t memWrite(thandle_t handle, tdata_t buffer, tsize_t size) {
        return 0;
    }

    toff_t memSeek(thandle_t handle, toff_t offset, int whence) {
        MemoryStream* stream = (MemoryStream*)handle;
        toff_t new_offset;
        switch (whence) {
            case SEEK_SET:
                new_offset = offset;
                break;
            case SEEK_CUR:
                new_offset = stream->offset + offset;
                break;
            case SEEK_END:
                new_offset = stream->size + offset;
                break;
            default:
                return -1;
        }
        if (new_offset < 0 || new_offset > stream->size) {
            return -1; // Неприпустима позиція
        }
        stream->offset = new_offset;
        return new_offset;
    }

    int memClose(thandle_t handle) {
        return 0;
    }

    toff_t memSize(thandle_t handle) {
        MemoryStream* stream = (MemoryStream*)handle;
        return stream->size;
    }

    int memMap(thandle_t handle, tdata_t* base, toff_t* size) {
        return 0;
    }

    void memUnmap(thandle_t handle, tdata_t base, toff_t size) {
    }

    BitmapPointer* openTIFF(uint8_t *tiffData, uint64_t length) {
        MemoryStream stream = { tiffData, length, 0 };

        TIFF* tiff = TIFFClientOpen(
                "InMemoryTIFF", "r", (thandle_t)&stream,
                memRead, memWrite, memSeek, memClose, memSize,
                memMap, memUnmap
        );

        if (!tiff) {
            return nullptr;
        }

        int32_t width, height;
        uint16_t samplesPerPixel, bitsPerSample;

        TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
        TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);

        if (samplesPerPixel != 1 && samplesPerPixel != 3) {
            TIFFClose(tiff);
            return nullptr;
        }

        uint16_t* buffer = new uint16_t[width * height * 3];
        uint8_t* scanline = new uint8_t[TIFFScanlineSize(tiff)];

        for (size_t row = 0; row < height; ++row) {
            if (TIFFReadScanline(tiff, scanline, row) < 0) {
                TIFFClose(tiff);
                delete [] scanline;
                return nullptr;
            }

            for (size_t col = 0; col < width; ++col) {
                size_t offset = col * samplesPerPixel;
                size_t outOffset = (row * width + col) * 3;

                if (samplesPerPixel == 1) {
                    uint16_t value;

                    if (bitsPerSample <= 8) {
                        value = (uint16_t)scanline[offset] * 257;
                    }
                    else if (bitsPerSample <= 16) {
                        uint16_t* scanline16 = reinterpret_cast<uint16_t*>(scanline);
                        value = scanline16[offset];

                        if (bitsPerSample < 16) {
                            value = value << (16 - bitsPerSample);
                        }
                    }

                    buffer[outOffset + 0] = value;
                    buffer[outOffset + 1] = value;
                    buffer[outOffset + 2] = value;
                }
                else {
                    uint16_t* scanline16 = reinterpret_cast<uint16_t*>(scanline);
                    if (bitsPerSample <= 8) {
                        buffer[outOffset] = static_cast<uint16_t>(scanline[offset]) * 257;
                        buffer[outOffset + 1] = static_cast<uint16_t>(scanline[offset + 1]) * 257;
                        buffer[outOffset + 2] = static_cast<uint16_t>(scanline[offset + 2]) * 257;
                    }
                    else {
                        buffer[outOffset] = bitsPerSample == 16 ? scanline16[offset] : scanline16[offset] << (16 - bitsPerSample);
                        buffer[outOffset + 1] = bitsPerSample == 16 ? scanline16[offset + 1] : scanline16[offset + 1] << (16 - bitsPerSample);
                        buffer[outOffset + 2] = bitsPerSample == 16 ? scanline16[offset + 2] : scanline16[offset + 2] << (16 - bitsPerSample);
                    }
                }
            }
        }

        delete [] scanline;
        TIFFClose(tiff);

        return new BitmapPointer {
                width,
                height,
                buffer
        };
    }

    void saveTIFF(int fd, Bitmap *bmp) {
        TIFF* tiff = TIFFFdOpen(fd, "OutputTIFF", "w");
        if (!tiff) {
            return;
        }

        TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, bmp->width);
        TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, bmp->height);
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 16);
        TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 3);
        TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
        TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, bmp->height);

        tsize_t scanlineSize = TIFFScanlineSize(tiff);
        size_t bmpStride = bmp->width * 3;
        uint16_t* scanline = new uint16_t[bmpStride];
        for (size_t row = 0; row < bmp->height; row++) {
            memcpy(scanline, bmp->buffer + row * bmpStride, scanlineSize);

            if (TIFFWriteScanline(tiff, scanline, row, 0) < 0) {
                delete[] scanline;
                TIFFCleanup(tiff);
                return;
            }
        }

        delete[] scanline;
        TIFFCleanup(tiff);
    }
}