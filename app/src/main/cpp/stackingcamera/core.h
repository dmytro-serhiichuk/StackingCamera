//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_CORE_H
#define STACKINGCAMERA_CORE_H

#include <cstdint>
#include "jni-helper.h"
#include "m-brisk/m-brisk.h"
#include "collections/list.h"

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
    extern uint32_t CHUNKS_COUNT;
    extern uint32_t CHUNKS_PER_SIDE;
    extern uint32_t KEYPOINTS_PER_CHUNK;
    extern uint32_t MATCHES_PER_CHUNK;
    extern float BRISK_PATTERN_SCALE_FACTOR;
    extern bool DRAW_KEYPOINTS;
    extern bool DRAW_MATCHES;
    extern ColorSpace BITMAP_COLOR_SPACE;
    extern Depth BITMAP_DEPTH;

    extern M_BRISK* mBrisk;
    extern List<Data>* sources;
    extern Bitmap* stackedResult;

    void init(AAssetManager* aam);
    void loadBitmap(int fd);
    void applySettings(int fast_threshold, float ransac_threshold, int ransac_iterations,
                       int chunks_per_side, int max_keypoints, int max_matches,
                       float brisk_pattern_scale, bool use16_bit_bitmaps, bool use_images,
                       bool use_rgb_images, bool use16_bit_images, bool draw_keypoints,
                       bool draw_matches);
    void analyse(bool reanalyse);
    void stack(bool disableAlignment);
    void save(int fd, int format);
}

#endif //STACKINGCAMERA_CORE_H
