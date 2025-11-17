//
// Created by sedv2 on 10.01.2025.
//

#include <string>
#include <unistd.h>

#include "pngio.h"

#include <png.h>

namespace ImageIO {
    BitmapPointer* openPNG(uint8_t *pngData, uint64_t length) {
        png_image image;
        memset(&image, 0, sizeof(image));
        image.version = PNG_IMAGE_VERSION;

        if (!png_image_begin_read_from_memory(&image, pngData, length)) {
            return nullptr;
        }

        image.format = PNG_FORMAT_LINEAR_RGB;

        int32_t width = static_cast<int32_t>(image.width);
        int32_t height = static_cast<int32_t>(image.height);

        uint64_t bufferSize = PNG_IMAGE_SIZE(image) / 2;
        uint16_t* buffer = new uint16_t[bufferSize];

        if (!png_image_finish_read(&image, nullptr, buffer, 0, nullptr)) {
            png_image_free(&image);
            return nullptr;
        }

        double gamma = 1.0 / 2.2;
        for (int i = 0; i < bufferSize; i++) {
            buffer[i] = std::pow(buffer[i] / 65535.0, gamma) * 65535.0;
        }

        png_image_free(&image);

        return new BitmapPointer {
            width,
            height,
            buffer
        };
    }

    void PngWriteToMemory(png_structp png_ptr, png_bytep data, png_size_t length) {
        std::vector<png_byte>* writer = static_cast<std::vector<png_byte>*>(png_get_io_ptr(png_ptr));
        writer->insert(writer->end(), data, data + length);
    }

    void savePNG(int fd, Bitmap* bitmap) {
        if (bitmap == nullptr) {
            return;
        }

        std::vector<png_byte> writer;
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (!png_ptr) {
            return;
        }

        png_infop info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
            png_destroy_write_struct(&png_ptr, nullptr);
            return;
        }

        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            return;
        }

        png_set_write_fn(png_ptr, &writer, PngWriteToMemory, nullptr);

        png_set_IHDR(png_ptr, info_ptr, bitmap->width, bitmap->height,
                     16, bitmap->colorType == ColorType::RGB ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        png_write_info(png_ptr, info_ptr);

        std::vector<png_bytep> row_pointers(bitmap->height);
        int k = bitmap->colorType == ColorType::RGB ? 3 : 1;
        for (int32_t y = 0; y < bitmap->height; ++y) {
            row_pointers[y] = reinterpret_cast<png_bytep>(bitmap->buffer + y * bitmap->width * k);
        }

        png_set_swap(png_ptr);

        png_write_image(png_ptr, row_pointers.data());
        png_write_end(png_ptr, nullptr);
        png_destroy_write_struct(&png_ptr, &info_ptr);

        write(fd, writer.data(), writer.size() * sizeof(png_byte));
    }
}