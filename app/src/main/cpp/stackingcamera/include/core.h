//
// Created by sedv2 on 14.01.2025.
//


#ifndef IMAGESTACKER_CORE_H
#define IMAGESTACKER_CORE_H

#include "list.h"
#include "bitmap-pointer.h"
#include "jniHelper.h"
#include "brisk.h"

namespace Core {
    enum class ExportTypes {
        JPEG = 0,
        PNG = 1,
        TIFF = 2
    };

    extern JNIHelper *jniHelper;

    extern int32_t FAST_THRESHOLD;
    extern float RANSAC_THRESHOLD;
    extern uint32_t RANSAC_ITERATIONS;
    extern uint32_t CHUNKS_COUNT;
    extern uint32_t CHUNKS_PER_SIDE;
    extern uint32_t KEYPOINTS_PER_CHUNK;
    extern uint32_t MATCHES_PER_CHUNK;

    extern List<BitmapPointer>* bitmaps;
    extern List<Buffer<KeyPoint>>* keyPoints;
    extern List<Descriptors>* descriptors;

    void init();

    void setSettings(
        int32_t fastThreshold,
        float ransacThreshold,
        uint32_t ransacIterations,
        uint32_t chunkPerSide,
        uint32_t maxKeyPoints,
        uint32_t maxMatches,
        float briskPatternScale,
        bool drawKeyPoints,
        bool drawMatches
    );

    void removeByIndex(int32_t index);

    void analyse(bool reanalyse);

    void stack(bool useAlignment);

    void saveResult(int32_t fd, ExportTypes type);
}

#endif //IMAGESTACKER_CORE_H

