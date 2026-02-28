//
// Created by sedv2 on 13.01.2026.
//

#include "core.h"
#include "stacking/matching.h"
#include "stacking/ransac.h"
#include "stacking/warping.h"
#include "stacking/median-stacking.h"
#include "utils.h"

namespace Core {
    enum class ImageFormat {
        NONE    = 0,
        RGB_8   = 1,
        RGB_16  = 2,
        RGBA_8  = 3,
        RGBA_16 = 4
    };

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
    ColorSpace BITMAP_COLOR_SPACE = ColorSpace::RGB;
    Depth BITMAP_DEPTH = Depth::U16;

    M_BRISK* mBrisk = nullptr;
    List<Data>* sources = new List<Data>(10);
    Bitmap* stackedResult = nullptr;

    int32_t referenceFrameIndex = 0;

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
                       int tiles_per_side, int max_keypoints, int max_matches,
                       float brisk_pattern_scale, bool use16_bit, bool use_images,
                       int image_format, bool save_keypoints, bool save_matches) {
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

        BITMAP_DEPTH = use16_bit ? Depth::U16 : Depth::U8;
        BITMAP_COLOR_SPACE = ColorSpace::RGB;
        // TODO: handle images
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
                    JNIHelper::getInstance()->writeMessageToLog(false, "Starting analysis of image %zd", i);

                    Bitmap* bitmap   = src->bitmapPtr->read();
                    src->keyPoints   = mBrisk->detect(*bitmap);
                    src->descriptors = mBrisk->compute(*bitmap, *src->keyPoints);
                    JNIHelper::getInstance()->writeMessageToLog(false, "\tDescriptors computing completed");

                    if (SAVE_KEYPOINTS) {
                        Utils::drawKeyPoints(*bitmap, *src->keyPoints);
                        JNIHelper::getInstance()->writeMessageToLog(false, "\tDrawing completed");
                    }

                    delete bitmap;
                    JNIHelper::getInstance()->writeMessageToLog(false, "Analysis of image %zd completed\n", i);
                }
                catch (std::exception &e) {
                    JNIHelper::getInstance()->writeMessageToLog(true, "Analysis of image %zd failed: %s\n", i, e.what());
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
        }

        updateReferenceFrameIndex();
        JNIHelper::getInstance()->writeMessageToLog(false, "Index of the reference frame: %d\n", referenceFrameIndex);

        Buffer<Buffer<Matching::Match>> *matches = Matching::match(*sources, referenceFrameIndex);

        if (SAVE_MATCHES) {
            Utils::drawAllMatches(*matches, referenceFrameIndex);
        }

        Buffer<Eigen::Matrix3d> *matrices = RANSAC::computeHomographyMatrices(*sources, referenceFrameIndex, *matches);
        delete matches;

        auto warpManager = new WarpManager(*sources->buffer[referenceFrameIndex]->bitmapPtr);
        List<BitmapPtr> *warpedBitmaps = warpManager->warp(*sources, referenceFrameIndex, *matrices);
        delete warpManager;
        delete matrices;

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