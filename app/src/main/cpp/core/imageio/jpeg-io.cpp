//
// Created by sedv2 on 10.01.2026.
//

#include "jpeg-io.h"
#include <turbojpeg.h>
#include <stdexcept>
#include <unistd.h>
#include "profiles-manager.h"

namespace ImageIO {
    namespace {
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

        ICCProfileData retrieveICCProfile(tjhandle decompressor) {
            ICCProfileData icc {};

            if (tj3GetICCProfile(decompressor, &icc.buffer, &icc.size) < 0 &&
                tj3GetErrorCode(decompressor) != 0) {
                tj3Destroy(decompressor);
                icc.free();
                throw std::runtime_error("Failed to get icc profile");
            }

            return icc;
        }
    }

    BitmapPtr *loadJPEG(const uint8_t *fileData, size_t fileSize, ColorSpace colorSpace, Depth depth) {
        tjhandle decompressor = tj3Init(TJINIT_DECOMPRESS);
        if (decompressor == nullptr) {
            throw std::runtime_error("Failed to init jpeg decompressor");
        }
        tj3Set(decompressor, TJPARAM_SAVEMARKERS, 2);

        if (tj3DecompressHeader(decompressor, fileData, fileSize) < 0) {
            tj3Destroy(decompressor);
            throw std::runtime_error("Failed to decompress jpeg header");
        }

        uint32_t width     = tj3Get(decompressor, TJPARAM_JPEGWIDTH);
        uint32_t height    = tj3Get(decompressor, TJPARAM_JPEGHEIGHT);
        TJCS jpgColorSpace = (TJCS)tj3Get(decompressor, TJPARAM_COLORSPACE);

        if (jpgColorSpace != TJCS_RGB && jpgColorSpace != TJCS_YCbCr) {
            tj3Destroy(decompressor);
            throw std::runtime_error("Failed to decompress jpeg (only RGB format is supported)");
        }

        auto icc = retrieveICCProfile(decompressor);
        bool isIccValid = icc.isValid();
        ColorSpace srcColorSpace = isIccValid ? ColorSpace::Other : ColorSpace::sRGB;

        const TJPF pixelFormat = TJPF_RGB;
        const ColorModel colorModel = ColorModel::RGB;

        size_t bufferSize = width * height * getSamplesPerPixel(colorModel);
        auto buffer = new uint8_t[bufferSize];

        if (tj3Decompress8(decompressor, fileData, fileSize, buffer, 0, pixelFormat) < 0) {
            tj3Destroy(decompressor);
            icc.free();
            throw std::runtime_error("Failed to decompress jpeg");
        }

        tj3Destroy(decompressor);

        auto bmp = Bitmap { width, height, buffer, Depth::U8, colorModel, srcColorSpace };

        if (srcColorSpace == colorSpace && depth != Depth::U8) {
            bmp = bmp.convertToRgbWithDepth(depth);
        } else if (srcColorSpace != colorSpace) {
            cmsHPROFILE profile = isIccValid
                    ? cmsOpenProfileFromMem(icc.buffer, icc.size)
                    : cmsCreate_sRGBProfile();
            bmp = bmp.convert(depth, colorSpace, profile);
            cmsCloseProfile(profile);
        }

        icc.free();
        return new BitmapPtr(bmp);
    }

    void saveJPEG(int fd, Bitmap &bmp, SaveProperties props) {
        Bitmap _converted {};
        Bitmap* bitmapPtr = nullptr;
        if (bmp.depth != Depth::U8 && bmp.colorSpace == ColorSpace::sRGB) {
            _converted = bmp.convertToRgbWithDepth(Depth::U8);
            bitmapPtr = &_converted;
        } else if (bmp.colorSpace != ColorSpace::sRGB) {
            auto p = createProfileFromColorSpace(bmp.colorSpace);
            _converted = bmp.convert(Depth::U8, ColorSpace::sRGB, p);
            cmsCloseProfile(p);
            bitmapPtr = &_converted;
        } else {
            bitmapPtr = &bmp;
        }

        unsigned long jpegSize = 0;
        uint8_t* jpegBuf = nullptr;

        tjhandle jpegCompressor = tj3Init(TJINIT_COMPRESS);
        if (!jpegCompressor) {
            throw std::runtime_error("Failed to init jpeg compressor");
        }

        jpegSize = 0;
        jpegBuf = nullptr;

        int pixelFormat = TJPF_RGB;
        int subsamp = TJSAMP_444;
        int colorSpace = TJCS_RGB;

        tj3Set(jpegCompressor, TJPARAM_QUALITY, props.jpegQuality);
        tj3Set(jpegCompressor, TJPARAM_SUBSAMP , subsamp);
        tj3Set(jpegCompressor, TJPARAM_COLORSPACE, colorSpace);

        auto profile = getProfileFromColorSpace(bitmapPtr->colorSpace);
        tj3SetICCProfile(jpegCompressor, profile.data(), profile.size());

        if (tj3Compress8(jpegCompressor, bitmapPtr->buffer, (int)bitmapPtr->width, 0,
                         (int)bitmapPtr->height, pixelFormat, &jpegBuf, &jpegSize) < 0) {
            tj3Destroy(jpegCompressor);
            throw std::runtime_error("Failed to compress jpeg");
        }

        tj3Destroy(jpegCompressor);
        write(fd, jpegBuf, jpegSize);
        tjFree(jpegBuf);
    }
}

