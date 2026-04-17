//
// Created by sedv2 on 17.04.2026.
//

#ifndef STACKINGCAMERA_SETTINGS_H
#define STACKINGCAMERA_SETTINGS_H

#include <cstdint>
#include "imageio/bitmap.h"

using namespace ImageIO;

class Settings {
public:
    static int32_t FAST_THRESHOLD;
    static float RANSAC_THRESHOLD;
    static uint32_t TILES_COUNT;
    static uint32_t TILES_PER_SIDE;
    static uint32_t KEYPOINTS_PER_TILE;
    static uint32_t MATCHES_PER_TILE;
    static uint32_t RANSAC_ITERATIONS;
    static float BRISK_PATTERN_SCALE_FACTOR;
    static bool SAVE_KEYPOINTS;
    static bool SAVE_MATCHES;
    static ColorSpace BITMAP_COLOR_SPACE;
    static Depth BITMAP_DEPTH;

    Settings() = delete;

    static void (*onBriskUpdated) ();

    static void update(int fast_threshold, float ransac_threshold, int ransac_iterations,
                int tiles_per_side, int max_keypoints, int max_matches,
                float brisk_pattern_scale, bool use16_bit,
                int color_space, bool save_keypoints, bool save_matches);
};

#endif //STACKINGCAMERA_SETTINGS_H
