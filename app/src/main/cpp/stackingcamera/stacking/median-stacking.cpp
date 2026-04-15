//
// Created by sedv2 on 14.01.2026.
//

#include "median-stacking.h"
#include "core.h"
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

Bitmap *MedianStacking::stack(List<BitmapPtr> &src, BitmapPtr &referenceBitmap) {
    auto outputBuffer = new uint8_t[referenceBitmap.bufferSize];
    auto depth = (size_t)referenceBitmap.depth;
    auto values = new uint8_t[src.size * depth];

    size_t chunkSize = Core::MAX_MEMORY_SIZE / src.size;
    size_t chunkCount = chunkSize / depth;
    auto chunks = new uint8_t*[src.size];
    size_t offset = 0;

    size_t bufferLength = referenceBitmap.bufferSize / depth;

    for (size_t i = 0; i < bufferLength; i++) {
        if (i % chunkCount == 0) {
            offset = i;
            for (size_t bi = 0; bi < src.size; bi++) {
                if (i != 0) delete [] chunks[bi];
                chunks[bi] = src.buffer[bi]->readChunk(offset * depth, chunkSize);
            }
        }

        if (referenceBitmap.depth == Depth::U8) {
            for (size_t j = 0; j < src.size; j++) {
                values[j] = chunks[j][i - offset];
            }
            outputBuffer[i] = findMedian8(values, src.size);
        } else {
            auto values16 = (uint16_t*)values;
            for (size_t j = 0; j < src.size; j++) {
                values16[j] = ((uint16_t*)chunks[j])[i - offset];
            }
            ((uint16_t*)outputBuffer)[i] = findMedian16(values16, src.size);
        }
    }

    for (size_t i = 0; i < src.size; i++) delete [] chunks[i];
    delete [] chunks;
    delete [] values;

    return new Bitmap(
            referenceBitmap.width,
            referenceBitmap.height,
            outputBuffer,
            referenceBitmap.depth,
            referenceBitmap.colorModel,
            referenceBitmap.colorSpace
    );
}
