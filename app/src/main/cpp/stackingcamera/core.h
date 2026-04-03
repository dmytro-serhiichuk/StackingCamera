//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_CORE_H
#define STACKINGCAMERA_CORE_H

#include <cstdint>
#include "jni-helper.h"
#include "m-brisk/m-brisk.h"
#include "collections/list.h"
#include <vector>
#include "matching/validation/validation.h"

using namespace ImageIO;

namespace Core {
    typedef struct Data {
        BitmapPtr* bitmapPtr;
        Buffer<KeyPoint>* keyPoints;
        Descriptors* descriptors;

        void removeAnalysedData();
    } Data;

    // TODO: make it depended on the available device memory
    constexpr size_t MAX_MEMORY_SIZE = 1342177280;  // 1.25 GB temp

    extern int32_t FAST_THRESHOLD;
    extern float RANSAC_THRESHOLD;
    extern uint32_t RANSAC_ITERATIONS;
    extern uint32_t TILES_COUNT;
    extern uint32_t TILES_PER_SIDE;
    extern uint32_t KEYPOINTS_PER_TILE;
    extern uint32_t MATCHES_PER_TILE;
    extern float BRISK_PATTERN_SCALE_FACTOR;
    extern bool SAVE_KEYPOINTS;
    extern bool SAVE_MATCHES;
    extern ColorSpace BITMAP_COLOR_SPACE;
    extern Depth BITMAP_DEPTH;

    extern M_BRISK* mBrisk;
    extern List<Data>* sources;
    extern Bitmap* stackedResult;

    void init(AAssetManager* aam);
    void loadBitmap(int fd);
    void applySettings(int fast_threshold, float ransac_threshold, int ransac_iterations,
                       int tiles_per_side, int max_keypoints, int max_matches,
                       float brisk_pattern_scale, bool use16_bit,
                       int color_space, bool save_keypoints, bool save_matches);
    int32_t updateReferenceFrameIndex();
    void removeBitmapAt(int32_t index);
    void analyse(bool reanalyse);
    std::vector<Validation::ValidationInfo> match();
    void stack(bool disableAlignment);
    void save(int fd, int format);
}

#endif //STACKINGCAMERA_CORE_H
