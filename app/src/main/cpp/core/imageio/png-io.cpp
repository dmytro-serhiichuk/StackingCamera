//
// Created by sedv2 on 10.01.2026.
//

#include "png-io.h"
#include <png.h>
#include <stdexcept>
#include <cmath>
#include <unistd.h>
#include "profiles-manager.h"

namespace ImageIO {
    namespace {
        typedef struct {
            const uint8_t *data;
            size_t size;
            size_t pos;
        } PngReadState;

        void png_read_from_memory(png_structp png_ptr, png_bytep out, png_size_t count) {
            auto state = (PngReadState *)png_get_io_ptr(png_ptr);

            if (state->pos + count > state->size) {
                png_error(png_ptr, "Read error: not enough data");
                return;
            }

            memcpy(out, state->data + state->pos, count);
            state->pos += count;
        }

        inline bool resolveColorModel(uint8_t colorSpace, ColorModel &colorModel) {
            if (colorSpace == PNG_COLOR_TYPE_RGB) {
                colorModel = ColorModel::RGB;
            } else if (colorSpace == PNG_COLOR_TYPE_RGB_ALPHA) {
                colorModel = ColorModel::RGBA;
            } else {
                return false;
            }
            return true;
        }

        inline cmsHPROFILE retrieveICCProfile(png_structp png, png_infop info, ColorSpace &colorSpace) {
            png_charp    icc_name;
            int          icc_compression;
            png_bytep    icc_data;
            png_uint_32  icc_length;

            if (png_get_iCCP(png, info, &icc_name, &icc_compression, &icc_data, &icc_length) == PNG_INFO_iCCP) {
                colorSpace = ColorSpace::Other;
                return cmsOpenProfileFromMem(icc_data, icc_length);
            }
            if (png_get_sRGB(png, info, nullptr) == PNG_INFO_sRGB) {
                colorSpace = ColorSpace::sRGB;
                return cmsCreate_sRGBProfile();
            }

            double wx, wy, rx, ry, gx, gy, bx, by;
            double gamma_value;

            png_uint_32 has_chrm = png_get_cHRM(png, info,
                                        &wx, &wy, &rx, &ry, &gx, &gy, &bx, &by) & PNG_INFO_cHRM;
            png_uint_32 has_gama = png_get_gAMA(png, info,
                                        &gamma_value) & PNG_INFO_gAMA;

            if (has_chrm && has_gama) {
                cmsCIExyYTRIPLE primaries = {
                        .Red   = { rx, ry, 1.0 },
                        .Green = { gx, gy, 1.0 },
                        .Blue  = { bx, by, 1.0 }
                };
                cmsCIExyY white_point = { wx, wy, 1.0 };

                cmsToneCurve *curve = cmsBuildGamma(nullptr, 1.0 / gamma_value);
                cmsToneCurve *curves[3] = { curve, curve, curve };

                colorSpace = ColorSpace::Other;

                return cmsCreateRGBProfile(
                        &white_point, &primaries, curves
                );
            } else if (has_chrm) { // assume sRGB gamma
                cmsCIExyYTRIPLE primaries = {
                        { rx, ry, 1.0 }, { gx, gy, 1.0 }, { bx, by, 1.0 }
                };
                cmsCIExyY white_point = { wx, wy, 1.0 };

                cmsToneCurve *srgb_trc = cmsBuildParametricToneCurve(
                        nullptr, 4, (double[]){ 2.4, 1.0/1.055, 0.055/1.055, 1.0/12.92, 0.04045 }
                );
                cmsToneCurve *curves[3] = { srgb_trc, srgb_trc, srgb_trc };

                colorSpace = ColorSpace::Other;

                return cmsCreateRGBProfile(
                        &white_point, &primaries, curves
                );
            } else {
                colorSpace = ColorSpace::sRGB;
                return cmsCreate_sRGBProfile();
            }
        }
    }

    BitmapPtr *loadPNG(const uint8_t *fileData, size_t fileSize, ColorSpace colorSpace, Depth depth) {
        auto png = png_create_read_struct(
                PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr
        );
        if (!png) {
            throw std::runtime_error("Failed to create png struct");
        }

        auto info = png_create_info_struct(png);
        if (!info) {
            png_destroy_read_struct(&png, nullptr, nullptr);
            throw std::runtime_error("Failed to create png info struct");
        }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_read_struct(&png, &info, nullptr);
            throw std::runtime_error("libpng error: setjmp");
        }

        PngReadState state = { fileData, fileSize, 0 };
        png_set_read_fn(png, &state, png_read_from_memory);
        png_read_info(png, info);

        uint8_t bit_depth  = png_get_bit_depth(png, info);
        uint8_t color_type = png_get_color_type(png, info);

        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png);
        }
        if (bit_depth == 16) {
            png_set_swap(png);
        }

        png_read_update_info(png, info);

        uint32_t width    = png_get_image_width(png, info);
        uint32_t height   = png_get_image_height(png, info);
        size_t row_stride = png_get_rowbytes(png, info);
        bit_depth         = png_get_bit_depth(png, info);
        color_type        = png_get_color_type(png, info);

        Depth srcDepth = bit_depth == 8 ? Depth::U8 : Depth::U16;
        ColorModel colorModel;
        if (!resolveColorModel(color_type, colorModel)) {
            png_destroy_read_struct(&png, &info, nullptr);
            throw std::runtime_error("Failed to decompress png (only RGB/RGBA formats are supported)");
        }

        ColorSpace srcColorSpace;
        cmsHPROFILE icc = retrieveICCProfile(png, info, srcColorSpace);

        size_t bufferSize = row_stride * height;
        auto buffer = new uint8_t [bufferSize];
        auto rows = new uint8_t *[height];

        for (int y = 0; y < height; y++)
            rows[y] = buffer + y * row_stride;

        png_read_image(png, rows);
        png_read_end(png, info);
        delete [] rows;
        png_destroy_read_struct(&png, &info, nullptr);

        auto bmp = Bitmap { width, height, buffer, srcDepth, colorModel, srcColorSpace };

        if (srcColorSpace == colorSpace && srcDepth != depth) {
            bmp = bmp.convertDepth(depth);
        } else if (srcColorSpace != colorSpace) {
            bmp = bmp.normalize(depth, colorSpace, icc);
        }
        cmsCloseProfile(icc);

        return new BitmapPtr(bmp);
    }

    void savePNG(int fd, Bitmap &bmp, SaveProperties props) {
        Bitmap _converted {};
        Bitmap* bitmapPtr = nullptr;
        if (bmp.colorSpace != ColorSpace::sRGB) {
            auto p = createProfileFromColorSpace(bmp.colorSpace);
            _converted = bmp.normalize(bmp.depth, ColorSpace::sRGB, p);
            cmsCloseProfile(p);
            bitmapPtr = &_converted;
        } else {
            bitmapPtr = &bmp;
        }

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png) {
            throw std::runtime_error("PNG: png_create_write_struct failed");
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_write_struct(&png, nullptr);
            throw std::runtime_error("PNG: png_create_info_struct failed");
        }

        FILE* file = fdopen(fd, "w");
        if (!file) {
            png_destroy_write_struct(&png, &info);
            throw std::runtime_error("PNG: cannot open file");
        }

        if (setjmp(png_jmpbuf(png))) {
            fclose(file);
            png_destroy_write_struct(&png, &info);
            throw std::runtime_error("PNG: libpng error during write");
        }

        png_init_io(png, file);

        int bitDepth = (int)bitmapPtr->depth * 8;

        png_set_IHDR(
                png, info, bitmapPtr->width, bitmapPtr->height, bitDepth, PNG_COLOR_TYPE_RGB,
                PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                PNG_FILTER_TYPE_DEFAULT
        );

        auto icc = getProfileFromColorSpace(bitmapPtr->colorSpace);
        png_set_iCCP(
            png, info, "ICC Profile", PNG_COMPRESSION_TYPE_BASE,
            reinterpret_cast<png_const_bytep>(icc.data()),
            static_cast<png_uint_32>(icc.size())
        );

        png_write_info(png, info);

        if (bitmapPtr->depth == Depth::U16) {
            png_set_swap(png);
        }

        const uint8_t* bufPtr = bitmapPtr->buffer;
        const auto rowBytes = static_cast<size_t>(bitmapPtr->stride);

        std::vector<png_bytep> rows(bitmapPtr->height);
        for (uint32_t y = 0; y < bitmapPtr->height; ++y) {
            rows[y] = const_cast<png_bytep>(bufPtr + y * rowBytes);
        }

        png_write_image(png, rows.data());
        png_write_end(png, nullptr);
        png_destroy_write_struct(&png, &info);

        fclose(file);
    }
}
