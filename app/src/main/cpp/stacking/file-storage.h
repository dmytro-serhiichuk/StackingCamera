//
// Created by sedv2 on 17.04.2026.
//

#ifndef STACKING_FILE_STORAGE_H
#define STACKING_FILE_STORAGE_H

#include <core/file-storage.h>

class JniFileStorage : public IFileStorage {
public:
    char* createTempFile() override;
    int createImageFile(const char *fileName) override;
};

#endif //STACKING_FILE_STORAGE_H
