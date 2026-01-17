//
// Created by sedv2 on 13.01.2026.
//

#include "core.h"
#include "stacking/matching.h"
#include "stacking/ransac.h"
#include "stacking/warping.h"
#include "stacking/median-stacking.h"

namespace Core {
    void Data::removeAnalysedData() {
        delete keyPoints;
        delete descriptors;
        keyPoints = nullptr;
        descriptors = nullptr;
    }

    int32_t FAST_THRESHOLD = 0;
    float RANSAC_THRESHOLD = .0f;
    uint32_t RANSAC_ITERATIONS = 0;
    uint32_t CHUNKS_COUNT = 0;
    uint32_t CHUNKS_PER_SIDE = 0;
    uint32_t KEYPOINTS_PER_CHUNK = 0;
    uint32_t MATCHES_PER_CHUNK = 0;
    float BRISK_PATTERN_SCALE_FACTOR = .0f;
    bool DRAW_KEYPOINTS = false;
    bool DRAW_MATCHES = false;
    ColorSpace BITMAP_COLOR_SPACE = ColorSpace::RGB;
    Depth BITMAP_DEPTH = Depth::U16;

    JNIHelper* jniHelper = nullptr;

    M_BRISK* mBrisk = nullptr;
    List<Data>* sources = new List<Data>(10);
    Bitmap* stackedResult = nullptr;

    int32_t bestBitmapIndex = 0;

    void init(AAssetManager *aam) {
        CL::init(aam);
    }

    void loadBitmap(int fd) {
        auto bitmapPtr = open(fd, BITMAP_COLOR_SPACE, BITMAP_DEPTH);
        auto data = new Data();
        data->bitmapPtr = bitmapPtr;
        sources->add(data);
    }

    void applySettings(int fast_threshold, float ransac_threshold, int ransac_iterations,
                       int chunks_per_side, int max_keypoints, int max_matches,
                       float brisk_pattern_scale, bool use16_bit_bitmaps, bool use_images,
                       bool use_rgb_images, bool use16_bit_images, bool draw_keypoints,
                       bool draw_matches) {
        FAST_THRESHOLD = fast_threshold;
        RANSAC_THRESHOLD = ransac_threshold;
        RANSAC_ITERATIONS = ransac_iterations;
        CHUNKS_PER_SIDE = chunks_per_side;
        CHUNKS_COUNT = CHUNKS_PER_SIDE * CHUNKS_PER_SIDE;
        KEYPOINTS_PER_CHUNK = std::ceil((float)max_keypoints / (float)CHUNKS_COUNT);
        MATCHES_PER_CHUNK = std::ceil((float)max_matches / (float)CHUNKS_COUNT);

        if (BRISK_PATTERN_SCALE_FACTOR != brisk_pattern_scale || mBrisk == nullptr) {
            BRISK_PATTERN_SCALE_FACTOR = brisk_pattern_scale;
            delete mBrisk;
            mBrisk = new M_BRISK(8, BRISK_PATTERN_SCALE_FACTOR);
        }

        BITMAP_DEPTH = use16_bit_bitmaps ? Depth::U16 : Depth::U8;
        // TODO: handle images
        DRAW_KEYPOINTS = draw_keypoints;
        DRAW_MATCHES = draw_matches;
    }

    static void updateBestBitmapIndex() {
        size_t n = 0;
        for (int32_t i = 0; i < sources->size; i++) {
            if (sources->buffer[i]->keyPoints->size > n) {
                n = sources->buffer[i]->keyPoints->size;
                bestBitmapIndex = i;
            }
        }
    }

    void analyse(bool reanalyse) {
        delete stackedResult;
        stackedResult = nullptr;

        if (reanalyse) {
            for (size_t i = 0; i < sources->size; i++) {
                sources->buffer[i]->removeAnalysedData();
            }
        }

        for (size_t i = 0; i < sources->size; i++) {
            auto src = sources->buffer[i];
            if (src->keyPoints == nullptr && src->descriptors == nullptr) {
                try {
                    Bitmap* bitmap   = src->bitmapPtr->read();
                    src->keyPoints   = mBrisk->detect(*bitmap);
                    src->descriptors = mBrisk->compute(*bitmap, *src->keyPoints);
                    delete bitmap;
                }
                catch (std::exception &e) {
                    // TODO: notify user about the error
                }
            }
        }
    }

    static void stackWithoutAlignment() {
        size_t bufferSize = sources->buffer[0]->bitmapPtr->bufferSize;
        auto stackedSrc = List<BitmapPtr>(sources->size, false);
        for (size_t i = 0; i < sources->size; i++) {
            if (sources->buffer[i]->bitmapPtr->bufferSize != bufferSize) {
                throw std::runtime_error("Images have to be the same size to be stacked without alignment");
            }
            stackedSrc.add(sources->buffer[i]->bitmapPtr);
        }
        stackedResult = MedianStacking::stack(stackedSrc, *sources->buffer[0]->bitmapPtr);
    }

    void stack(bool disableAlignment) {
        if (sources->size < 2) {
            throw std::runtime_error("Stacking requires at least 2 images");
        }

        if (disableAlignment) {
            stackWithoutAlignment();
            return;
        }

        updateBestBitmapIndex();

        Buffer<Buffer<Matching::Match>> *matches = Matching::match(*sources, bestBitmapIndex);
        Buffer<Eigen::Matrix3d> *matrices = RANSAC::computeHomographyMatrices(*sources, bestBitmapIndex, *matches);
        delete matches;

        auto warpManager = new WarpManager(*sources->buffer[bestBitmapIndex]->bitmapPtr);
        List<BitmapPtr> *warpedBitmaps = warpManager->warp(*sources, bestBitmapIndex, *matrices);
        delete warpManager;
        delete matrices;

        auto stackedSrc = List<BitmapPtr>(sources->size, false);
        stackedSrc.add(sources->buffer[bestBitmapIndex]->bitmapPtr);
        for (size_t i = 0; i < warpedBitmaps->size; i++) {
            stackedSrc.add(warpedBitmaps->buffer[i]);
        }

        stackedResult = MedianStacking::stack(stackedSrc, *sources->buffer[bestBitmapIndex]->bitmapPtr);
        delete warpedBitmaps;
    }

    void save(int fd, int format) {
        if (stackedResult == nullptr) throw std::runtime_error("There is no stacked bitmap");

        auto saveProperties = ImageIO::SaveProperties{};
        saveProperties.outputFormat = (ImageIO::OutputFormat)format;
        ImageIO::save(fd, *stackedResult, saveProperties);
    }
}