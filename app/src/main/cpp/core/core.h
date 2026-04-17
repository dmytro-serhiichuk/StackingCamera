//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_CORE_H
#define STACKINGCAMERA_CORE_H

#include <cstdint>
#include "imageio/imageio.h"
#include "m-brisk/m-brisk.h"
#include "collections/list.h"
#include <vector>
#include "matching/validation/validation.h"
#include "logger.h"
#include "file-storage.h"
#include "settings.h"

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

    extern M_BRISK* mBrisk;
    extern List<Data>* sources;
    extern Bitmap* stackedResult;

    void init(ILogger* logger, IFileStorage* fileStorage);
    ILogger *getLogger();
    IFileStorage *getFileStorage();

    void loadBitmap(int fd);

    int32_t updateReferenceFrameIndex();
    void removeBitmapAt(int32_t index);
    void analyse(bool reanalyse);
    std::vector<Validation::ValidationInfo> match();
    void stack(bool disableAlignment);
    void save(int fd, int format);
}

#endif //STACKINGCAMERA_CORE_H
