//
// Created by sedv2 on 14.01.2025.
//

#include "core.h"
#include "ransac.h"
#include "warp.h"
#include "pngio.h"
#include "jpegio.h"
#include "utils.h"
#include "tiff-io.h"

namespace Core {
    const size_t MAX_MEMORY_SIZE = 1342177280; // 1.25 GB temp

    JNIHelper *jniHelper = nullptr;

    int32_t FAST_THRESHOLD = 20;
    float RANSAC_THRESHOLD = 1.0f;
    uint32_t RANSAC_ITERATIONS = 6000;
    uint32_t CHUNKS_COUNT = 36;
    uint32_t CHUNKS_PER_SIDE = 6;
    uint32_t KEYPOINTS_PER_CHUNK = 20000;
    uint32_t MATCHES_PER_CHUNK = 500;
    float BRISK_PATTERN_SCALE = 8.0f;

    bool DRAW_KEYPOINTS = false;
    bool DRAW_MATCHES = false;

    List<BitmapPointer>* bitmaps = nullptr;
    List<Buffer<KeyPoint>>* keyPoints = nullptr;
    List<Descriptors>* descriptors = nullptr;

    Bitmap* stackedResult = nullptr;

    BRISK* brisk = nullptr;

    void init() {
        bitmaps = new List<BitmapPointer>();
        keyPoints = new List<Buffer<KeyPoint>>();
        descriptors = new List<Descriptors>();

        brisk = new BRISK(8, BRISK_PATTERN_SCALE);
    }
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
    ) {
        FAST_THRESHOLD = fastThreshold;
        RANSAC_THRESHOLD = ransacThreshold;
        RANSAC_ITERATIONS = ransacIterations;

        CHUNKS_PER_SIDE = chunkPerSide;
        CHUNKS_COUNT = CHUNKS_PER_SIDE * CHUNKS_PER_SIDE;

        KEYPOINTS_PER_CHUNK = std::ceil(maxKeyPoints / (float)CHUNKS_COUNT);
        MATCHES_PER_CHUNK = std::ceil(maxMatches / (float)CHUNKS_COUNT);

        if (briskPatternScale != BRISK_PATTERN_SCALE) {
            BRISK_PATTERN_SCALE = briskPatternScale;
            delete brisk;
            brisk = new BRISK(8, BRISK_PATTERN_SCALE);
        }

        DRAW_KEYPOINTS = drawKeyPoints;
        DRAW_MATCHES = drawMatches;
    }

    void removeByIndex(int32_t index) {
        if (index < 0 || index >= bitmaps->size) return;

        bitmaps->removeAt(index);
        keyPoints->removeAt(index);
        descriptors->removeAt(index);
    }

    void analyse(bool reanalyse) {
        if (stackedResult != nullptr) {
            delete stackedResult;
            stackedResult = nullptr;
        }

        if (reanalyse) {
            if (keyPoints != nullptr) delete keyPoints;
            if (descriptors != nullptr) delete descriptors;
            keyPoints = new List<Buffer<KeyPoint>>(bitmaps->size);
            descriptors = new List<Descriptors>(bitmaps->size);
        }

        for (int i = keyPoints->size; i < bitmaps->size; i++) {
            jniHelper->progressMessage = "Analysing: " + std::to_string(i + 1) + "/" + std::to_string(bitmaps->size);
            jniHelper->setProgressMessage();

            Bitmap *bitmap = bitmaps->buffer[i]->read();

            Buffer<KeyPoint>* kps = brisk->detect(*bitmap);
            Descriptors* des = brisk->compute(*bitmap, *kps);
            keyPoints->add(kps);
            descriptors->add(des);

            if (DRAW_KEYPOINTS) {
                jniHelper->progressMessage += "\nDrawing Descriptors...";
                jniHelper->setProgressMessage();

                Utils::drawKeyPoints(*bitmap, *keyPoints->buffer[i]);
            }

            delete bitmap;
        }
    }
    uint32_t findBestBitmap() {
        uint32_t bestIndex = 0;
        uint32_t bestScore = keyPoints->buffer[0]->size;
        for (uint32_t i = 1; i < keyPoints->size; i++) {
            if (keyPoints->buffer[i]->size > bestScore) {
                bestIndex = i;
                bestScore = keyPoints->buffer[i]->size;
            }
        }
        return bestIndex;
    }

    Buffer<Eigen::Matrix3d> *findHomographes(Buffer<Buffer<Match>> &matches, uint32_t bestIndex) {
        Buffer<Eigen::Matrix3d>* matrices = new Buffer<Eigen::Matrix3d>(matches.size);
        matrices->size = matches.size;
        size_t matchesIndex = 0;
        for (size_t i = 0; i < keyPoints->size; i++) {
            if (i == bestIndex) continue;
            jniHelper->progressMessage = "Finding Homography: " + std::to_string(matchesIndex + 1) + "/" + std::to_string(matches.size);
            jniHelper->setProgressMessage();

            matrices->buffer[matchesIndex] = RANSAC::computeHomography(
                    matches.buffer[matchesIndex],
                    *keyPoints->buffer[i],
                    *keyPoints->buffer[bestIndex]
            );
            matchesIndex++;
        }
        return matrices;
    }

    List<BitmapPointer> *warpBitmaps(Buffer<Eigen::Matrix3d> &matrices, uint32_t bestIndex) {
        List<BitmapPointer> *warpedBitmaps = new List<BitmapPointer>(descriptors->size - 1);

        Bitmap *bestBitmap = bitmaps->buffer[bestIndex]->read();
        Warper *warper = new Warper(bestBitmap);

        size_t matrixIndex = 0;
        for (size_t i = 0; i < descriptors->size; i++) {
            if (i != bestIndex) {
                jniHelper->progressMessage = "Warping: " + std::to_string(warpedBitmaps->size + 1) + "/" + std::to_string(matrices.size);
                jniHelper->setProgressMessage();

                Bitmap *bitmap = bitmaps->buffer[i]->read();

                BitmapPointer *warped = warper->warpPerspective(bitmap, matrices[matrixIndex]);
                warpedBitmaps->add(warped);
                matrixIndex++;

                delete bitmap;
            }
        }

        delete warper;
        delete bestBitmap;

        return warpedBitmaps;
    }

    uint16_t findMedian(uint16_t *values) {
        std::sort(values, values + bitmaps->size, [](const uint16_t &a, const uint16_t &b) {
              return a < b;
        });

        if (bitmaps->size % 2 == 0) {
            return (values[bitmaps->size / 2 - 1] + values[bitmaps->size / 2]) / 2;
        }
        else {
            return values[bitmaps->size / 2];
        }
    }

    void stackBitmaps(BitmapPointer &bestBitmapPointer, List<BitmapPointer> &sources) {
        uint16_t *outputBuffer = new uint16_t[bestBitmapPointer.bufferLength];
        uint16_t values [bitmaps->size];

        Bitmap* bestBitmap = bestBitmapPointer.read();

        size_t chunkSize = MAX_MEMORY_SIZE / sources.size / sizeof(uint16_t);
        List<uint16_t>* buffers = new List<uint16_t>(sources.size);
        size_t offset = 0;

        for (size_t i = 0; i < bestBitmapPointer.bufferLength; i++) {
            if (i % chunkSize == 0) {
                offset = i;
                delete buffers;
                buffers = new List<uint16_t>(sources.size);
                for (size_t bi = 0; bi < sources.size; bi++) {
                    buffers->add(sources.buffer[bi]->readChunk(i, chunkSize));
                }
            }
            values[0] = bestBitmap->buffer[i];
            for (size_t j = 0; j < sources.size; j++) {
                values[j + 1] = buffers->buffer[j][i - offset];
            }
            outputBuffer[i] = findMedian(values);
        }

        delete buffers;
        delete bestBitmap;

        stackedResult = new Bitmap {
                bestBitmapPointer.width,
                bestBitmapPointer.height,
                outputBuffer
        };
    }

    void stackWithoutAlignment() {
        if (bitmaps->size < 2) {
            throw std::runtime_error("The number of bitmaps must exceed 2");
        }

        jniHelper->progressMessage = "Stacking...";
        jniHelper->setProgressMessage();

        stackBitmaps(*bitmaps->buffer[0], *bitmaps);
    }

    void stack(bool useAlignment) {
        if (!useAlignment) {
            stackWithoutAlignment();
            return;
        }

        if (keyPoints->size < 2) {
            throw std::runtime_error("The number of bitmaps must exceed 2");
        }

        uint32_t bestIndex = findBestBitmap();

        jniHelper->progressMessage = "Finding Matches...";
        jniHelper->setProgressMessage();

        Buffer<Buffer<Match>> *matches = Matcher::match(*bitmaps->buffer[bestIndex], *descriptors, *keyPoints->buffer[bestIndex], bestIndex);
        if (DRAW_MATCHES) Utils::drawAllMatches(*matches, bestIndex);

        jniHelper->progressMessage = "Computing Homographes...";
        jniHelper->setProgressMessage();

        Buffer<Eigen::Matrix3d> *matrices = findHomographes(*matches, bestIndex);
        delete matches;

        jniHelper->progressMessage = "Warping images...";
        jniHelper->setProgressMessage();

        List<BitmapPointer> *warpedBitmaps = warpBitmaps(*matrices, bestIndex);
        delete matrices;

        jniHelper->progressMessage = "Stacking...";
        jniHelper->setProgressMessage();

        stackBitmaps(*bitmaps->buffer[bestIndex], *warpedBitmaps);
        delete warpedBitmaps;
    }

    void saveResult(int32_t fd, ExportTypes type) {
        if (stackedResult == nullptr) return;

        if (type == ExportTypes::JPEG) {
            ImageIO::saveJPEG(fd, Core::stackedResult, 100);
        }
        else if (type == ExportTypes::PNG) {
            ImageIO::savePNG(fd, Core::stackedResult);
        }
        else if (type == ExportTypes::TIFF) {
            ImageIO::saveTIFF(fd, Core::stackedResult);
        }
    }
}