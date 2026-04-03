//
// Created by sedv2 on 10.01.2026.
//

#include "raw-i.h"
#include <libraw.h>
#include "profiles-manager.h"

namespace ImageIO {
    bool isRAW(void* buffer, size_t size) {
        LibRaw processor {};
        if (processor.open_buffer(buffer, size) == LIBRAW_SUCCESS) {
            processor.recycle();
            return true;
        }
        processor.recycle();
        return false;
    }

    BitmapPtr* loadRAW(uint8_t* fileData, size_t fileSize, ColorSpace colorSpace, Depth depth) {
        LibRaw processor {};
        // Open file from memory
        if (processor.open_buffer(fileData, fileSize) != LIBRAW_SUCCESS) {
            processor.recycle();
            throw std::runtime_error("RAW: Cannot open input file");
        }

        processor.imgdata.params.output_bps = 16;
        processor.imgdata.params.use_camera_wb = 1;
        processor.imgdata.params.use_camera_matrix = 1;
        processor.imgdata.params.no_auto_bright = 1;

        processor.imgdata.params.gamm[0] = 1.0;
        processor.imgdata.params.gamm[1] = 1.0;
        processor.imgdata.params.output_color = LIBRAW_COLORSPACE_ICC;

        // Unpack the raw data
        if (processor.unpack() != LIBRAW_SUCCESS) {
            processor.recycle();
            throw std::runtime_error("RAW: Cannot unpack input image");
        }
        // Postprocess the raw data to extract image
        if (processor.dcraw_process() != LIBRAW_SUCCESS) {
            processor.recycle();
            throw std::runtime_error("RAW: dcraw_process failed");
        }

        // Get the processed image data
        libraw_processed_image_t* processed_image = processor.dcraw_make_mem_image();
        if (!processed_image) {
            processor.recycle();
            throw std::runtime_error("RAW: Getting processed image failed");
        }

        auto profile = buildXYZ_d65();

        int32_t bigWidth = processed_image->width;
        int32_t bigHeight = processed_image->height;

        int32_t leftOffset   = processor.imgdata.sizes.raw_inset_crops[0].cleft - processor.imgdata.sizes.left_margin;
        int32_t topOffset    = processor.imgdata.sizes.raw_inset_crops[0].ctop - processor.imgdata.sizes.top_margin;

        int32_t cropWidth    = processor.imgdata.sizes.raw_inset_crops[0].cwidth;
        int32_t cropHeight   = processor.imgdata.sizes.raw_inset_crops[0].cheight;

        int32_t rightOffset  = bigWidth - cropWidth - leftOffset;
        int32_t bottomOffset = bigHeight - cropHeight - topOffset;

        auto orientation = processor.imgdata.sizes.flip;
        if (orientation == 3) { // 180 deg
            leftOffset = rightOffset;
            topOffset = bottomOffset;
        } else if (orientation == 5) { // 90 deg counter-clockwise
            leftOffset = topOffset;
            topOffset = rightOffset;
            int32_t cw = cropWidth;
            cropWidth = cropHeight;
            cropHeight = cw;
        } else if (orientation == 6) { // 90 deg clockwise
            topOffset = leftOffset;
            leftOffset = bottomOffset;
            int32_t cw = cropWidth;
            cropWidth = cropHeight;
            cropHeight = cw;
        }

        uint32_t outputWidth, outputHeight;
        uint16_t* buffer = nullptr;

        if (cropWidth != 0 && cropHeight != 0) {
            const size_t channels = 3;
            auto src = reinterpret_cast<uint16_t *>(processed_image->data);
            buffer = new uint16_t[cropWidth * cropHeight * channels];

            const size_t srcRow = bigWidth * channels;
            const size_t dstRow = cropWidth * channels;

            size_t srcOffset = topOffset * srcRow;

            const uint32_t leftSrcOffset = leftOffset * channels;

            for (size_t i = 0; i < cropHeight; i++) {
                memcpy(buffer + dstRow * i, src + srcOffset + leftSrcOffset, dstRow * sizeof(uint16_t));
                srcOffset += srcRow;
            }

            outputWidth = cropWidth;
            outputHeight = cropHeight;
        }
        else {
            buffer = new uint16_t[processed_image->data_size];
            std::memcpy(buffer, processed_image->data, processed_image->data_size * sizeof(uint8_t));

            outputWidth = bigWidth;
            outputHeight = bigHeight;
        }

        processor.dcraw_clear_mem(processed_image);
        processor.recycle();

        Bitmap bmp { outputWidth, outputHeight, buffer, Depth::U16, ColorModel::XYZ, ColorSpace::Other };
        bmp = bmp.normalize(depth, colorSpace, profile);
        cmsCloseProfile(profile);

        return new BitmapPtr(bmp);
    }
}
