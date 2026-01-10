//
// Created by sedv2 on 10.01.2026.
//

#include "raw-i.h"
#include <libraw.h>

namespace ImageIO {
    bool isRaw(void* buffer, size_t size) {
        LibRaw processor;
        if (processor.open_buffer(buffer, size) == LIBRAW_SUCCESS) {
            processor.recycle();
            return true;
        }
        processor.recycle();
        return false;
    }

    BitmapPtr* loadRAW(uint8_t* fileData, size_t fileSize, ColorSpace colorSpace, Depth depth) {
        LibRaw processor;
        // Open file from memory
        if (processor.open_buffer(fileData, fileSize) != LIBRAW_SUCCESS) {
            return nullptr;
        }

        // TODO: still bad brightness (especially .DNG)
        processor.imgdata.params.output_bps = depth == Depth::U16 ? 16 : 8;
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

        size_t bytesPerSample = depth == Depth::U16 ? sizeof(uint16_t) : sizeof(uint8_t);

        uint8_t *buffer = nullptr;
        size_t width, height;

        if (cropWidth != 0 && cropHeight != 0) {
            uint8_t *src = reinterpret_cast<uint8_t *>(processed_image->data);
            buffer = new uint8_t[cropWidth * cropHeight * 3 * bytesPerSample];
            size_t offset = topOffset * bigWidth * 3;

            uint32_t startOffset = leftOffset * 3 * bytesPerSample;
            uint32_t row = cropWidth * 3 * bytesPerSample;
            uint32_t backOffset = (bigWidth - cropWidth - leftOffset) * 3 * bytesPerSample;

            for (size_t i = 0; i < cropHeight; i++) {
                offset += startOffset;
                memcpy(buffer + row * i, src + offset + row * i, row * bytesPerSample);
                offset += backOffset;
            }

            width = cropWidth;
            height = cropHeight;
        }
        else {
            buffer = new uint8_t[bigWidth * bigHeight * 3 * bytesPerSample];
            std::memcpy(buffer, processed_image->data, processed_image->data_size);

            width = bigWidth;
            height = bigHeight;
        }

        processor.dcraw_clear_mem(processed_image);
        processor.recycle();

        Bitmap* bmp = new Bitmap(width, height, buffer, ColorSpace::RGB, depth);
        if (colorSpace != ColorSpace::RGB) {
            Bitmap* tmp = bmp;
            bmp = tmp->convertColor(colorSpace);
            delete tmp;
        }

        BitmapPtr *bitmapPointer = new BitmapPtr(*bmp);
        delete bmp;

        return bitmapPointer;
    }
}
