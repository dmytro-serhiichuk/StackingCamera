//
// Created by sedv2 on 14.01.2026.
//

#include "median-stacking.h"
#include "../core.h"
#include <algorithm>

namespace {
    uint16_t findMedian16(uint16_t *values, size_t size) {
        std::sort(values, values + size, [](const uint16_t &a, const uint16_t &b) {
            return a < b;
        });

        if (size % 2 == 0) return (values[size / 2 - 1] + values[size / 2]) / 2;
        else return values[size / 2];
    }
    uint8_t findMedian8(uint8_t *values, size_t size) {
        std::sort(values, values + size, [](const uint16_t &a, const uint16_t &b) {
            return a < b;
        });

        if (size % 2 == 0) return (values[size / 2 - 1] + values[size / 2]) / 2;
        else return values[size / 2];
    }
}

Bitmap *MedianStacking::stack(BitmapPtr &baseBitmapPtr, List<BitmapPtr> &src) {
    auto outputBuffer = new uint8_t[baseBitmapPtr.bufferSize];
    size_t depth = (size_t)baseBitmapPtr.depth;
    size_t valuesCount = src.size + 1;
    auto values = new uint8_t[valuesCount * depth];

    Bitmap *baseBitmap = baseBitmapPtr.read();

    size_t chunkSize = Core::MAX_MEMORY_SIZE / src.size;
    size_t chunkCount = chunkSize / depth;
    List<uint8_t> chunks {src.size};
    size_t offset = 0;

    for (size_t i = 0; i < baseBitmap->bufferLength; i++) {
        if (i % chunkCount == 0) {
            offset = i;
            chunks = List<uint8_t>(src.size);
            for (size_t bi = 0; bi < src.size; bi++) {
                chunks.add(src.buffer[bi]->readChunk(offset * depth, chunkSize));
            }
        }

        if (baseBitmap->depth == Depth::U8) {
            values[0] = baseBitmap->buffer[i];
            for (size_t j = 0; j < src.size; j++) {
                values[j + 1] = chunks.buffer[j][i - offset];
            }
            outputBuffer[i] = findMedian8(values, valuesCount);
        } else {
            auto values16 = (uint16_t*)values;
            values16[0] = ((uint16_t*)baseBitmap->buffer)[i];
            for (size_t j = 0; j < src.size; j++) {
                values16[j + 1] = ((uint16_t*)chunks.buffer[j])[i - offset];
            }
            ((uint16_t*)outputBuffer)[i] = findMedian16(values16, valuesCount);
        }
    }

    delete baseBitmap;

    return new Bitmap(baseBitmapPtr.width, baseBitmapPtr.height, outputBuffer, baseBitmapPtr.colorSpace, baseBitmapPtr.depth);
}
