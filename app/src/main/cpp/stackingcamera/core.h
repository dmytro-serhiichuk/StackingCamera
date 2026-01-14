//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_CORE_H
#define STACKINGCAMERA_CORE_H

#include <cstdint>
#include "jni-helper.h"

namespace Core {
    const size_t MAX_MEMORY_SIZE = 1342177280;  // 1.25 GB temp

    int32_t FAST_THRESHOLD = 0;
    float RANSAC_THRESHOLD = .0f;
    uint32_t RANSAC_ITERATIONS = 0;
    uint32_t CHUNKS_COUNT = 0;
    uint32_t CHUNKS_PER_SIDE = 0;
    uint32_t KEYPOINTS_PER_CHUNK = 0;
    uint32_t MATCHES_PER_CHUNK = 0;
    float BRISK_PATTERN_SCALE = .0f;
    bool DRAW_KEYPOINTS = false;
    bool DRAW_MATCHES = false;

    JNIHelper* jniHelper = nullptr;
}

#endif //STACKINGCAMERA_CORE_H
