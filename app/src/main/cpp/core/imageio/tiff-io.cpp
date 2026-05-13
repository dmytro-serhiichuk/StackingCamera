//
// Created by sedv2 on 10.01.2026.
//

#include "tiff-io.h"
#include <tiffio.h>
#include <stdexcept>
#include "profiles-manager.h"

namespace ImageIO {
    namespace {
        typedef struct {
            const unsigned char* data;
            toff_t size;
            toff_t pos;
        } MemTIFF;

        tsize_t mem_read(thandle_t handle, tdata_t buf, tsize_t size) {
            auto mem = (MemTIFF*)handle;

            if (mem->pos + size > mem->size) {
                size = mem->size - mem->pos;
            }

            memcpy(buf, mem->data + mem->pos, size);
            mem->pos += size;
            return size;
        }

        tsize_t mem_write(thandle_t handle, tdata_t buf, tsize_t size) {
            return 0;
        }

        toff_t mem_seek(thandle_t handle, toff_t off, int whence) {
            auto mem = (MemTIFF*)handle;
            switch (whence) {
                case SEEK_SET: mem->pos = off; break;
                case SEEK_CUR: mem->pos += off; break;
                case SEEK_END: mem->pos = mem->size + off; break;
            }
            return mem->pos;
        }

        int mem_close(thandle_t handle) {
            return 0;
        }

        toff_t mem_size(thandle_t handle) {
            auto mem = (MemTIFF*)handle;
            return mem->size;
        }

        int mem_map(thandle_t handle, tdata_t* pbase, toff_t* psize) {
            return 0;
        }

        void mem_unmap(thandle_t handle, tdata_t base, toff_t size) {}
    }

    namespace {
        struct AlphaInfo {
            bool hasAssocAlpha = false;
            bool hasUnassAlpha = false;
        };

        inline AlphaInfo getAlphaInfo(TIFF *tiff) {
            uint16_t  nExtra     = 0;
            uint16_t* extraTypes = nullptr;
            TIFFGetField(tiff, TIFFTAG_EXTRASAMPLES, &nExtra, &extraTypes);

            bool hasAssocAlpha = false; // pre-multiplied (EXTRASAMPLE_ASSOCALPHA)
            bool hasUnassAlpha = false; // straight       (EXTRASAMPLE_UNASSALPHA)
            for (uint16_t i = 0; i < nExtra; ++i) {
                if (extraTypes[i] == EXTRASAMPLE_ASSOCALPHA) hasAssocAlpha = true;
                if (extraTypes[i] == EXTRASAMPLE_UNASSALPHA)  hasUnassAlpha  = true;
            }

            return { hasAssocAlpha, hasUnassAlpha };
        }

        inline void readTiles(TIFF *tiff, Bitmap &bm, bool isSeparate, size_t bytesPerPixel, size_t bytesPerSample) {
            uint32_t tileW = 0, tileH = 0;
            TIFFGetField(tiff, TIFFTAG_TILEWIDTH,  &tileW);
            TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tileH);

            if (!isSeparate) {
                std::vector<uint8_t> tileBuf(TIFFTileSize(tiff));
                for (uint32_t ty = 0; ty < bm.height; ty += tileH) {
                    for (uint32_t tx = 0; tx < bm.width; tx += tileW) {
                        if (TIFFReadTile(tiff, tileBuf.data(), tx, ty, 0, 0) < 0)
                            throw std::runtime_error("TIFF: failed to read tile");

                        uint32_t copyW = std::min(tileW, bm.width - tx);
                        uint32_t copyH = std::min(tileH, bm.height - ty);
                        for (uint32_t row = 0; row < copyH; ++row) {
                            uint8_t* src = tileBuf.data() + row * tileW * bytesPerPixel;
                            uint8_t* dst = bm.buffer + (ty + row) * bm.width * bytesPerPixel
                                           + tx * bytesPerPixel;
                            std::memcpy(dst, src, copyW * bytesPerPixel);
                        }
                    }
                }
            } else {
                uint16_t samplePerPixel = getSamplesPerPixel(bm.colorModel);
                std::vector<uint8_t> tileBuf(TIFFTileSize(tiff));
                for (uint16_t s = 0; s < samplePerPixel; ++s) {
                    for (uint32_t ty = 0; ty < bm.height; ty += tileH) {
                        for (uint32_t tx = 0; tx < bm.width; tx += tileW) {
                            if (TIFFReadTile(tiff, tileBuf.data(), tx, ty, 0, s) < 0)
                                throw std::runtime_error("TIFF: failed to read planar tile");

                            uint32_t copyW = std::min(tileW, bm.width - tx);
                            uint32_t copyH = std::min(tileH, bm.height - ty);
                            for (uint32_t row = 0; row < copyH; ++row) {
                                for (uint32_t x = 0; x < copyW; ++x) {
                                    uint8_t* src = tileBuf.data()
                                                   + (row * tileW + x) * bytesPerSample;
                                    uint8_t* dst = bm.buffer
                                                   + ((ty + row) * bm.width + (tx + x)) * bytesPerPixel
                                                   + s * bytesPerSample;
                                    std::memcpy(dst, src, bytesPerSample);
                                }
                            }
                        }
                    }
                }
            }
        }

        inline void readStrip(TIFF *tiff, Bitmap &bm, bool isSeparate, size_t bytesPerPixel, size_t bytesPerSample) {
            if (!isSeparate) {
                for (uint32_t row = 0; row < bm.height; ++row) {
                    uint8_t* dst = bm.buffer + row * bm.width * bytesPerPixel;
                    if (TIFFReadScanline(tiff, dst, row) < 0)
                        throw std::runtime_error("TIFF: failed to read scanline");
                }
            } else {
                uint16_t samplesPerPixel = getSamplesPerPixel(bm.colorModel);
                std::vector<uint8_t> rowBuf(bm.width * bytesPerSample);
                for (uint16_t s = 0; s < samplesPerPixel; ++s) {
                    for (uint32_t row = 0; row < bm.height; ++row) {
                        if (TIFFReadScanline(tiff, rowBuf.data(), row, s) < 0)
                            throw std::runtime_error("TIFF: failed to read planar scanline");
                        for (uint32_t x = 0; x < bm.width; ++x) {
                            uint8_t* src = rowBuf.data() + x * bytesPerSample;
                            uint8_t* dst = bm.buffer + (row * bm.width + x) * bytesPerPixel
                                           + s * bytesPerSample;
                            std::memcpy(dst, src, bytesPerSample);
                        }
                    }
                }
            }
        }

        struct ICCProfileData {
            size_t size = 0;
            uint8_t* buffer = nullptr;

            [[nodiscard]] bool isValid() const {
                return size > 0 && buffer != nullptr;
            }
            void free() const {
                delete [] buffer;
            }
        };

        ICCProfileData retrieveICCProfile(TIFF* tiff) {
            ICCProfileData icc {};
            TIFFGetField(tiff, TIFFTAG_ICCPROFILE, &icc.size, &icc.buffer);
            return icc;
        }
    }


    BitmapPtr* loadTIFF(const uint8_t* fileData, size_t fileSize, ColorSpace colorSpace, Depth depth) {
        MemTIFF mem = { fileData, fileSize, 0 };

        TIFF* tiff = TIFFClientOpen("MEM_TIFF", "r", (thandle_t)&mem, mem_read, mem_write, mem_seek, mem_close, mem_size, mem_map, mem_unmap);
        if (!tiff) {
            throw std::runtime_error("Failed to open file");
        }

        struct Guard { TIFF* t; ~Guard() { TIFFClose(t); } } guard{tiff};

        Bitmap bmp {};
        if (!TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH,  &bmp.width) ||
            !TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &bmp.height))
            throw std::runtime_error("TIFF: missing width or height tag");

        uint16_t spp = 1;
        TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLESPERPIXEL, &spp);

        uint16_t bps = 1;
        TIFFGetFieldDefaulted(tiff, TIFFTAG_BITSPERSAMPLE, &bps);
        if (bps != 8 && bps != 16) {
            throw std::runtime_error("TIFF: Only images with 8/16 bits per sample are supported");
        }
        bmp.depth = bps == 8 ? Depth::U8 : Depth::U16;

        uint16_t sampleFormat;
        TIFFGetFieldDefaulted(tiff, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
        if (sampleFormat != SAMPLEFORMAT_UINT) {
            throw std::runtime_error("TIFF: Only images with uint sample type are supported");
        }

        AlphaInfo alphaInfo = getAlphaInfo(tiff);

        uint16_t photo = PHOTOMETRIC_RGB;
        if (!TIFFGetField(tiff, TIFFTAG_PHOTOMETRIC, &photo)) {
            throw std::runtime_error("TIFF: Photometric tas is not defined");
        }
        if (photo != PHOTOMETRIC_RGB) {
            throw std::runtime_error("TIFF: Only RGB images are supported");
        }

        bool hasAlpha = alphaInfo.hasAssocAlpha || alphaInfo.hasUnassAlpha;
        bmp.colorModel = hasAlpha ? ColorModel::RGBA : ColorModel::RGB;

        uint16_t planarConfig = PLANARCONFIG_CONTIG;
        TIFFGetFieldDefaulted(tiff, TIFFTAG_PLANARCONFIG, &planarConfig);
        const bool isSeparate = (planarConfig == PLANARCONFIG_SEPARATE);

        bmp.totalSamples = bmp.width * bmp.height * getSamplesPerPixel(bmp.colorModel);
        bmp.bufferSize = bmp.totalSamples * (size_t)bmp.depth;
        bmp.buffer = new uint8_t [bmp.bufferSize];

        auto bytesPerSample = (size_t)bmp.depth;
        size_t bytesPerPixel = bytesPerSample * getSamplesPerPixel(bmp.colorModel);

        if (TIFFIsTiled(tiff)) {
            readTiles(tiff, bmp, isSeparate, bytesPerPixel, bytesPerSample);
        } else {
            readStrip(tiff, bmp, isSeparate, bytesPerPixel, bytesPerSample);
        }

        if (alphaInfo.hasAssocAlpha) {
            const size_t numPixels = bmp.width * bmp.height;
            const int    alphaIdx       = spp - 1;
            const uint64_t maxVal       = (1ULL << bps) - 1;

            for (size_t p = 0; p < numPixels; ++p) {
                uint8_t* px = bmp.buffer + p * bytesPerPixel;

                uint64_t alpha = 0;
                std::memcpy(&alpha, px + alphaIdx * bytesPerSample, bytesPerSample);
                if (alpha == 0 || alpha == maxVal) continue;

                for (int s = 0; s < alphaIdx; ++s) {
                    uint64_t sample = 0;
                    std::memcpy(&sample, px + s * bytesPerSample, bytesPerSample);
                    uint64_t result = std::min((sample * maxVal + alpha / 2) / alpha, maxVal);
                    std::memcpy(px + s * bytesPerSample, &result, bytesPerSample);
                }
            }
        }

        auto icc = retrieveICCProfile(tiff);
        bool isIccValid = icc.isValid();
        bmp.colorSpace = isIccValid ? ColorSpace::Other : ColorSpace::sRGB;

        if (bmp.colorSpace == colorSpace && (bmp.depth != depth || bmp.colorModel != ColorModel::RGB)) {
            bmp = bmp.convertToRgbWithDepth(depth);
        } else if (bmp.colorSpace != colorSpace) {
            cmsHPROFILE profile = isIccValid
                                  ? cmsOpenProfileFromMem(icc.buffer, icc.size)
                                  : cmsCreate_sRGBProfile();
            bmp = bmp.convert(depth, colorSpace, profile);
            cmsCloseProfile(profile);
        }

        icc.free();
        return new BitmapPtr(bmp);
    }

    void saveTIFF(int fd, Bitmap &bmp, SaveProperties props) {
        Bitmap _converted {};
        Bitmap* bitmapPtr = nullptr;
        if (bmp.colorSpace != ColorSpace::sRGB) {
            auto p = createProfileFromColorSpace(bmp.colorSpace);
            _converted = bmp.convert(bmp.depth, ColorSpace::sRGB, p);
            cmsCloseProfile(p);
            bitmapPtr = &_converted;
        } else {
            bitmapPtr = &bmp;
        }

        TIFF* tiff = TIFFFdOpen(fd, "IMAGE", "w");
        if (!tiff) {
            throw std::runtime_error("Failed to open file");
        }

        uint16_t samplesPerPixel = 3;

        auto depthSize = (uint16_t)bitmapPtr->depth;
        uint16_t bitsPerSample = depthSize * 8;

        TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH,      bitmapPtr->width);
        TIFFSetField(tiff, TIFFTAG_IMAGELENGTH,     bitmapPtr->height);
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE,   bitsPerSample);
        TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, samplesPerPixel);
        TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT,    SAMPLEFORMAT_UINT);
        TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC,     PHOTOMETRIC_RGB);
        TIFFSetField(tiff, TIFFTAG_ORIENTATION,     ORIENTATION_TOPLEFT);
        TIFFSetField(tiff, TIFFTAG_PLANARCONFIG,    PLANARCONFIG_CONTIG);
        TIFFSetField(tiff, TIFFTAG_COMPRESSION,     COMPRESSION_NONE);

        auto icc = getProfileFromColorSpace(bitmapPtr->colorSpace);
        TIFFSetField(
                tiff, TIFFTAG_ICCPROFILE,
                static_cast<uint32_t>(icc.size()), icc.data()
        );

        auto rowStep = static_cast<size_t>(bitmapPtr->stride);

        for (uint32_t y = 0; y < bitmapPtr->height; ++y) {
            uint8_t* row = bitmapPtr->buffer + y * rowStep;

            if (TIFFWriteScanline(tiff, row, y, 0) < 0) {
                TIFFClose(tiff);
                throw std::runtime_error("TIFF: TIFFWriteScanline failed");
            }
        }

        TIFFClose(tiff);
    }
}
