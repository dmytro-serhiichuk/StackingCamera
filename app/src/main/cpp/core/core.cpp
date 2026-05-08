//
// Created by sedv2 on 13.01.2026.
//

#include "core.h"
#include "matching/matching.h"
#include "matching/ransac.h"
#include "stacking/warping.h"
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

    M_BRISK* mBrisk = nullptr;
    List<Data>* sources = new List<Data>(10);
    Bitmap* stackedResult = nullptr;

    int32_t referenceFrameIndex = 0;
    Buffer<RANSAC::Result> *homographies = nullptr;

    ILogger *_logger = nullptr;
    IFileStorage *_fileStorage = nullptr;

    void handleBriskUpdated() {
        delete mBrisk;
        mBrisk = new M_BRISK(Settings::BRISK_PATTERN_SCALE_FACTOR);
    }

    void init(ILogger *logger, IFileStorage *fileStorage) {
        _logger = logger;
        _fileStorage = fileStorage;

        Settings::onBriskUpdated = &handleBriskUpdated;
    }

    ILogger *getLogger() {
        return _logger;
    }
    IFileStorage *getFileStorage() {
        return _fileStorage;
    }

    void loadBitmap(int fd) {
        auto bitmapPtr = open(fd, Settings::BITMAP_COLOR_SPACE, Settings::BITMAP_DEPTH);
        auto data = new Data();
        data->bitmapPtr = bitmapPtr;
        sources->add(data);
        delete homographies;
        homographies = nullptr;
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
                    _logger->log(false, "Starting analysis of image %zd", i);

                    Bitmap bitmap    = src->bitmapPtr->read();
                    src->keyPoints   = mBrisk->detect(bitmap);
                    src->descriptors = mBrisk->compute(bitmap, *src->keyPoints);
                    _logger->log(false, "\tDescriptors computing completed");

                    if (Settings::SAVE_KEYPOINTS) {
                        Utils::drawKeyPoints(bitmap, *src->keyPoints);
                        _logger->log(false, "\tDrawing completed");
                    }

                    _logger->log(false, "Analysis of image %zd completed\n", i);
                }
                catch (std::exception &e) {
                    _logger->log(true, "Analysis of image %zd failed: %s\n", i, e.what());
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

        if (Settings::SAVE_MATCHES) Utils::drawAllMatches(*matches, referenceFrameIndex);

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
        _logger->log(false, "Starting stacking");
        stackedResult = Settings::STACKING_METHOD->stack(stackedSrc, *sources->buffer[0]->bitmapPtr);
        _logger->log(false, "Stacking completed");
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

        _logger->log(false, "Starting stacking");
        stackedResult = Settings::STACKING_METHOD->stack(stackedSrc, *sources->buffer[referenceFrameIndex]->bitmapPtr);
        delete warpedBitmaps;
        _logger->log(false, "Stacking completed");
    }

    void save(int fd, int format) {
        if (stackedResult == nullptr) throw std::runtime_error("There is no stacked bitmap");

        auto saveProperties = ImageIO::SaveProperties{};
        saveProperties.outputFormat = (ImageIO::OutputFormat)format;
        ImageIO::save(fd, *stackedResult, saveProperties);
    }
}