//
// Created by sedv2 on 05.01.2025.
//

#include <cstdint>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

#include "imageio.h"
#include "tiff-io.h"
#include "pngio.h"
#include "jpegio.h"

#include <libraw.h>

namespace ImageIO {
    enum class ImageType {
        RAW,
        JPG,
        PNG,
        TIFF,
        UNDEFINED
    };

    uint8_t* readFile(int fd, uint64_t& length) {
        struct stat fileStat;
        if (fstat(fd, &fileStat) == -1) {
            return nullptr;
        }

        length = fileStat.st_size;
        if (length <= 0) {
            return nullptr;
        }

        uint8_t* buffer = new uint8_t[length];

        ssize_t bytesRead = read(fd, buffer, length);
        if (bytesRead < 0) {
            delete[] buffer;
            return nullptr;
        }

        return buffer;
    }

    ImageType getImageType(const uint8_t *fileData, uint64_t length) {
        if (length < 8) {
            return  ImageType::UNDEFINED;
        }

        LibRaw processor;
        if (processor.open_buffer(fileData, length) == LIBRAW_SUCCESS) {
            processor.recycle();
            return ImageType::RAW;
        }

        if (fileData[0] == 0xFF && fileData[1] == 0xD8 && fileData[2] == 0xFF) {
            return ImageType::JPG;
        }

        if (fileData[0] == 0x89 && fileData[1] == 0x50 && fileData[2] == 0x4E && fileData[3] == 0x47 &&
            fileData[4] == 0x0D && fileData[5] == 0x0A && fileData[6] == 0x1A && fileData[7] == 0x0A) {
            return ImageType::PNG;
        }

        // Little Endian (49 49 2A 00)
        if (fileData[0] == 0x49 && fileData[1] == 0x49 && fileData[2] == 0x2A && fileData[3] == 0x00) {
            return ImageType::TIFF;
        }
        // Big Endian (4D 4D 00 2A)
        if (fileData[0] == 0x4D && fileData[1] == 0x4D && fileData[2] == 0x00 && fileData[3] == 0x2A) {
            return ImageType::TIFF;
        }

        return ImageType::UNDEFINED;
    }

    BitmapPointer* openRAW(uint8_t *fileData, uint64_t length) {
        LibRaw processor;
        // Open file from memory
        if (processor.open_buffer(fileData, length) != LIBRAW_SUCCESS) {
            return nullptr;
        }

        // TODO: still bad brightness (especially .DNG)
        processor.imgdata.params.output_bps = 16;
        processor.imgdata.params.use_camera_wb = 1;
        processor.imgdata.params.output_tiff = 1;
        processor.imgdata.params.no_auto_bright = 0;
        processor.imgdata.params.auto_bright_thr = 0.0;

        processor.imgdata.params.output_color = 1; // sRGB

        // Unpack the raw data
        if (processor.unpack() != LIBRAW_SUCCESS) {
            return nullptr;
        }
        // Postprocess the raw data to extract image
        if (processor.dcraw_process() != LIBRAW_SUCCESS) {
            return nullptr;
        }

        // Get the processed image data
        libraw_processed_image_t* processed_image = processor.dcraw_make_mem_image();
        if (!processed_image) {
            return nullptr;
        }

        int32_t bigWidth = processed_image->width;
        int32_t bigHeight = processed_image->height;

        bool wh = bigWidth > bigHeight;

        // TODO: use correct orientation (start/back offsets can be different)
        int32_t leftOffset = wh ?
                processor.imgdata.sizes.raw_inset_crops[0].cleft - processor.imgdata.sizes.left_margin :
                processor.imgdata.sizes.raw_inset_crops[0].ctop - processor.imgdata.sizes.top_margin;

        int32_t topOffset = wh ?
                processor.imgdata.sizes.raw_inset_crops[0].ctop - processor.imgdata.sizes.top_margin :
                processor.imgdata.sizes.raw_inset_crops[0].cleft - processor.imgdata.sizes.left_margin;

        int32_t cropWidth = wh ?
                processor.imgdata.sizes.raw_inset_crops[0].cwidth :
                processor.imgdata.sizes.raw_inset_crops[0].cheight;
        int32_t cropHeight = wh ?
                processor.imgdata.sizes.raw_inset_crops[0].cheight :
                processor.imgdata.sizes.raw_inset_crops[0].cwidth;

        BitmapPointer *bitmapPointer = nullptr;

        if (cropWidth != 0 && cropHeight != 0) {
            uint16_t *src = reinterpret_cast<uint16_t *>(processed_image->data);
            uint16_t *buffer = new uint16_t[cropWidth * cropHeight * 3];
            size_t offset = topOffset * bigWidth * 3;

            uint32_t startOffset = leftOffset * 3;
            uint32_t row = cropWidth * 3;
            uint32_t backOffset = (bigWidth - cropWidth - leftOffset) * 3;

            for (size_t i = 0; i < cropHeight; i++) {
                offset += startOffset;
                memcpy(buffer + row * i, src + offset + row * i, row * sizeof(uint16_t));
                offset += backOffset;
            }

            bitmapPointer = new BitmapPointer {
                    cropWidth,
                    cropHeight,
                    buffer
            };
        }
        else {
            uint16_t* buffer = new uint16_t[processed_image->data_size];
            std::memcpy(buffer, processed_image->data, processed_image->data_size * sizeof(uint8_t));

            bitmapPointer = new BitmapPointer {
                    bigWidth,
                    bigHeight,
                    buffer
            };
        }

        processor.dcraw_clear_mem(processed_image);
        processor.recycle();

        return bitmapPointer;
    }

    BitmapPointer *openImage(int fd) {
        uint64_t length = 0;
        uint8_t* fileData = readFile(fd, length);

        if (fileData == nullptr || length <= 0) {
            return nullptr;
        }

        ImageType imageType = getImageType(fileData, length);

        BitmapPointer* bmp;

        switch (imageType) {
            case ImageType::JPG:
                bmp = openJPEG(fileData, length);
                break;
            case ImageType::PNG:
                bmp = openPNG(fileData, length);
                break;
            case ImageType::TIFF:
                bmp = openTIFF(fileData, length);
                break;
            case ImageType::RAW:
                bmp = openRAW(fileData, length);
                break;
            default:
                bmp = nullptr;
        }

        delete[] fileData;

        return bmp;
    }
}