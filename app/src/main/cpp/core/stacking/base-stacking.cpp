//
// Created by sedv2 on 08.05.2026.
//

#include "base-stacking.h"
#include "core.h"

Bitmap *BaseStacking::stack(List<BitmapPtr> &src, BitmapPtr &referenceBitmap) {
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
            outputBuffer[i] = get8(values, src.size);
        } else {
            auto values16 = (uint16_t*)values;
            for (size_t j = 0; j < src.size; j++) {
                values16[j] = ((uint16_t*)chunks[j])[i - offset];
            }
            ((uint16_t*)outputBuffer)[i] = get16(values16, src.size);
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
