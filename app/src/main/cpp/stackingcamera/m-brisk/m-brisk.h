//
// Created by sedv2 on 11.01.2026.
//

#ifndef STACKINGCAMERA_M_BRISK_H
#define STACKINGCAMERA_M_BRISK_H


#include "imageio/imageio.h"
#include "collections/buffer.h"
#include "opencl.h"

using namespace ImageIO;

typedef struct KeyPoint {
    float x;
    float y;
    int32_t response;
    uint32_t octave;
} KeyPoint;

typedef struct BriskPatternPoint
{
    float x;
    float y;
    float sigma;
} BriskPatternPoint;

typedef struct BriskShortPair
{
    uint32_t i;
    uint32_t j;
} BriskShortPair;

typedef struct BriskLongPair
{
    uint32_t i;
    uint32_t j;
    int32_t weighted_dx;
    int32_t weighted_dy;
} BriskLongPair;

typedef struct Descriptors {
    static constexpr uint32_t DESCRIPTOR_LENGTH = 8; // 512 (size of descriptor) / 64 (size of array element) = 8

    uint64_t* buffer;
    size_t count;

    ~Descriptors();

    [[nodiscard]] inline size_t sizeOf() const {
        return count * DESCRIPTOR_LENGTH * sizeof(uint64_t);
    }
} Descriptors;

class M_BRISK {
private:
    static constexpr uint32_t nOctaves = 8;
    const float OCTAVE_SCALE_FACTOR = 0.7071067811865475f;
    const uint32_t MIN_OCTAVE_SIZE = 256;
    const uint32_t FAST_PADDING = 15;
    const size_t MAX_KEYPOINTS = 500000;

    float* scales;
    uint32_t* sizes;
    BriskPatternPoint* patternPoints;   // [point][rotation][octave]
    const uint32_t nPoints = 60;
    const uint32_t nRotations = 1024;
    static constexpr float dScaleRange = 3.75f;    // 30 / 8 - default value
    static constexpr float scaleRange = dScaleRange * (float)nOctaves;
    const float dMaxSq = 5.85f * 5.85f;
    const float dMinSq = 8.2f * 8.2f;
    Buffer<BriskShortPair>* shortPairs;
    Buffer<BriskLongPair>* longPairs;

    inline void subpixelRefine(const Bitmap &bitmap, cl_mem buffer, Buffer<KeyPoint> &keypoints);
    static inline bool RoiPredicate(const Bitmap &bitmap, const KeyPoint &kp, uint32_t size);
    inline void filterKeypointsAfterRefining(const Bitmap &bmp, Buffer<KeyPoint> &kps);
public:
    explicit M_BRISK(float _briskScaleFactor=1.4f);
    ~M_BRISK();

    Buffer<KeyPoint>* detect(const Bitmap &inputBitmap);
    Descriptors* compute(const Bitmap &inputBitmap, Buffer<KeyPoint> &keyPoints);
};

#endif //STACKINGCAMERA_M_BRISK_H
