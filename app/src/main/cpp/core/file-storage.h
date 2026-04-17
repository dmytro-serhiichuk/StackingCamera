//
// Created by sedv2 on 17.04.2026.
//

#ifndef CORE_FILE_STORAGE_H
#define CORE_FILE_STORAGE_H

class IFileStorage {
public:
    virtual ~IFileStorage() = default;
    virtual char* createTempFile() = 0;
    virtual int createImageFile(const char *fileName) = 0;
};

#endif //CORE_FILE_STORAGE_H
