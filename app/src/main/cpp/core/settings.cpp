//
// Created by sedv2 on 17.04.2026.
//

#include "settings.h"
#include "core.h"
#include <stdexcept>

uint32_t Settings::TILES_COUNT = 0;
uint32_t Settings::TILES_PER_SIDE = 0;
int32_t Settings::FAST_THRESHOLD = 0;
float Settings::RANSAC_THRESHOLD = .0f;
uint32_t Settings::KEYPOINTS_PER_TILE = 0;
uint32_t Settings::MATCHES_PER_TILE = 0;
uint32_t Settings::RANSAC_ITERATIONS = 0;
float Settings::BRISK_PATTERN_SCALE_FACTOR = 0;
bool Settings::SAVE_KEYPOINTS = false;
bool Settings::SAVE_MATCHES = false;
ColorSpace Settings::BITMAP_COLOR_SPACE = ColorSpace::Other;
Depth Settings::BITMAP_DEPTH = Depth::U16;

void (*Settings::onBriskUpdated)() = nullptr;

void Settings::update(int fast_threshold, float ransac_threshold, int ransac_iterations,
                      int tiles_per_side, int max_keypoints, int max_matches,
                      float brisk_pattern_scale, bool use16_bit, int color_space,
                      bool save_keypoints, bool save_matches) {
    auto newDepth = use16_bit ? Depth::U16 : Depth::U8;
    auto newColorSpace = (ColorSpace)color_space;

    if (Core::sources->size != 0 &&
        (newDepth != BITMAP_DEPTH || newColorSpace != BITMAP_COLOR_SPACE)) {
        throw std::runtime_error("Invalid settings update");
    }

    FAST_THRESHOLD = fast_threshold;
    RANSAC_THRESHOLD = ransac_threshold;
    RANSAC_ITERATIONS = ransac_iterations;
    TILES_PER_SIDE = tiles_per_side;
    TILES_COUNT = TILES_PER_SIDE * TILES_PER_SIDE;
    KEYPOINTS_PER_TILE = std::ceil((float)max_keypoints / (float)TILES_COUNT);
    MATCHES_PER_TILE = std::ceil((float)max_matches / (float)TILES_COUNT);

    if (BRISK_PATTERN_SCALE_FACTOR != brisk_pattern_scale) {
        BRISK_PATTERN_SCALE_FACTOR = brisk_pattern_scale;
        onBriskUpdated();
    }

    BITMAP_DEPTH = newDepth;
    BITMAP_COLOR_SPACE = newColorSpace;
    SAVE_KEYPOINTS = save_keypoints;
    SAVE_MATCHES = save_matches;
}
