//
// Created by sedv2 on 13.01.2026.
//

#include "core.h"
#include "matching/matching.h"
#include "matching/ransac.h"
#include "stacking/warping.h"
#include "stacking/median-stacking.h"
#include "utils.h"
#include "matching/validation/homography-validation.h"
#include "matching/validation/matches-validation.h"

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
    uint32_t TILES_COUNT = 0;
    uint32_t TILES_PER_SIDE = 0;
    uint32_t KEYPOINTS_PER_TILE = 0;
    uint32_t MATCHES_PER_TILE = 0;
    float BRISK_PATTERN_SCALE_FACTOR = .0f;
    bool SAVE_KEYPOINTS = false;
    bool SAVE_MATCHES = false;
    ColorSpace BITMAP_COLOR_SPACE = ColorSpace::sRGB;
    Depth BITMAP_DEPTH = Depth::U16;

    M_BRISK* mBrisk = nullptr;
    List<Data>* sources = new List<Data>(10);
    Bitmap* stackedResult = nullptr;

    int32_t referenceFrameIndex = 0;
    Buffer<RANSAC::Result> *homographies = nullptr;

    void init(AAssetManager *aam) {
        CL::init(aam);
    }

    void loadBitmap(int fd) {
        auto bitmapPtr = open(fd, BITMAP_COLOR_SPACE, BITMAP_DEPTH);
        auto data = new Data();
        data->bitmapPtr = bitmapPtr;
        sources->add(data);
        delete homographies;
        homographies = nullptr;
    }

    void applySettings(int fast_threshold, float ransac_threshold, int ransac_iterations,
                       int tiles_per_side, int max_keypoints, int max_matches,
                       float brisk_pattern_scale, bool use16_bit,
                       int color_space, bool save_keypoints, bool save_matches) {
        auto newDepth = use16_bit ? Depth::U16 : Depth::U8;

        if (TILES_PER_SIDE != tiles_per_side || BITMAP_DEPTH != newDepth) {
            for (size_t i = 0; i < sources->size; i++) {
                sources->buffer[i]->removeAnalysedData();
            }
            delete homographies;
            homographies = nullptr;
        }

        FAST_THRESHOLD = fast_threshold;
        RANSAC_THRESHOLD = ransac_threshold;
        RANSAC_ITERATIONS = ransac_iterations;
        TILES_PER_SIDE = tiles_per_side;
        TILES_COUNT = TILES_PER_SIDE * TILES_PER_SIDE;
        KEYPOINTS_PER_TILE = std::ceil((float)max_keypoints / (float)TILES_COUNT);
        MATCHES_PER_TILE = std::ceil((float)max_matches / (float)TILES_COUNT);

        if (BRISK_PATTERN_SCALE_FACTOR != brisk_pattern_scale || mBrisk == nullptr) {
            BRISK_PATTERN_SCALE_FACTOR = brisk_pattern_scale;
            delete mBrisk;
            mBrisk = new M_BRISK(8, BRISK_PATTERN_SCALE_FACTOR);
        }

        BITMAP_DEPTH = newDepth;
        BITMAP_COLOR_SPACE = (ColorSpace)color_space;
        SAVE_KEYPOINTS = save_keypoints;
        SAVE_MATCHES = save_matches;
    }

    int32_t updateReferenceFrameIndex() {
        size_t n = 0;
        for (int32_t i = 0; i < sources->size; i++) {
            if (sources->buffer[i]->keyPoints->size > n) {
                n = sources->buffer[i]->keyPoints->size;
                referenceFrameIndex = i;
            }
        }
        return referenceFrameIndex;
    }

    void removeBitmapAt(int32_t index) {
        sources->removeAt(index);
        if (index == referenceFrameIndex) {
            delete homographies;
            homographies = nullptr;
        }
    }

    void analyse(bool reanalyse) {
        delete stackedResult;
        stackedResult = nullptr;
        delete homographies;
        homographies = nullptr;

        if (reanalyse) {
            for (size_t i = 0; i < sources->size; i++) {
                sources->buffer[i]->removeAnalysedData();
            }
        }

        for (size_t i = 0; i < sources->size; i++) {
            auto src = sources->buffer[i];
            if (src->keyPoints == nullptr && src->descriptors == nullptr) {
                try {
                    JNIHelper::getInstance()->writeMessageToLog(false, "Starting analysis of image %zd", i);

                    Bitmap bitmap    = src->bitmapPtr->read();
                    src->keyPoints   = mBrisk->detect(bitmap);
                    src->descriptors = mBrisk->compute(bitmap, *src->keyPoints);
                    JNIHelper::getInstance()->writeMessageToLog(false, "\tDescriptors computing completed");

                    if (SAVE_KEYPOINTS) {
                        Utils::drawKeyPoints(bitmap, *src->keyPoints);
                        JNIHelper::getInstance()->writeMessageToLog(false, "\tDrawing completed");
                    }

                    JNIHelper::getInstance()->writeMessageToLog(false, "Analysis of image %zd completed\n", i);
                }
                catch (std::exception &e) {
                    JNIHelper::getInstance()->writeMessageToLog(true, "Analysis of image %zd failed: %s\n", i, e.what());
                }
            }
        }
    }

    std::vector<Validation::ValidationInfo> match() {
        if (sources->size < 2) {
            throw std::runtime_error("Matching requires at least 2 images");
        }

        updateReferenceFrameIndex();

        Buffer<Buffer<Matching::Match>> *matches = Matching::match(*sources, referenceFrameIndex);

        if (SAVE_MATCHES) Utils::drawAllMatches(*matches, referenceFrameIndex);

        homographies = RANSAC::computeHomographyMatrices(*sources, referenceFrameIndex, *matches);
        auto matchesValidationInfo = Validation::validateMatches(*sources, *matches, *homographies, referenceFrameIndex);
        delete matches;

        auto homographiesValidationInfo = Validation::validateMatrices(*sources, *homographies, referenceFrameIndex);

        std::vector<Validation::ValidationInfo> validationInfos {};
        validationInfos.reserve(matchesValidationInfo.size());

        int32_t index = 0;
        for (int32_t i = 0; i < sources->size; i++) {
            if (i == referenceFrameIndex) continue;
            validationInfos.push_back({
                homographiesValidationInfo[index],
                matchesValidationInfo[index],
                i
            });
            index++;
        }

        return validationInfos;
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
        JNIHelper::getInstance()->writeMessageToLog(false, "Starting median stacking");
        stackedResult = MedianStacking::stack(stackedSrc, *sources->buffer[0]->bitmapPtr);
        JNIHelper::getInstance()->writeMessageToLog(false, "Median stacking completed");
    }

    void stack(bool disableAlignment) {
        if (sources->size < 2) {
            throw std::runtime_error("Stacking requires at least 2 images");
        }

        if (disableAlignment) {
            stackWithoutAlignment();
            return;
        } else if (homographies == nullptr) {
            throw std::runtime_error("Matching was not performed");
        }

        auto warpManager = new WarpManager(*sources->buffer[referenceFrameIndex]->bitmapPtr);
        List<BitmapPtr> *warpedBitmaps = warpManager->warp(*sources, referenceFrameIndex, *homographies);
        delete warpManager;

        auto stackedSrc = List<BitmapPtr>(sources->size, false);
        stackedSrc.add(sources->buffer[referenceFrameIndex]->bitmapPtr);
        for (size_t i = 0; i < warpedBitmaps->size; i++) {
            stackedSrc.add(warpedBitmaps->buffer[i]);
        }

        JNIHelper::getInstance()->writeMessageToLog(false, "Starting median stacking");
        stackedResult = MedianStacking::stack(stackedSrc, *sources->buffer[referenceFrameIndex]->bitmapPtr);
        delete warpedBitmaps;
        JNIHelper::getInstance()->writeMessageToLog(false, "Median stacking completed");
    }

    void save(int fd, int format) {
        if (stackedResult == nullptr) throw std::runtime_error("There is no stacked bitmap");

        auto saveProperties = ImageIO::SaveProperties{};
        saveProperties.outputFormat = (ImageIO::OutputFormat)format;
        ImageIO::save(fd, *stackedResult, saveProperties);
    }
}