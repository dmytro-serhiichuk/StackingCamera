//
// Created by sedv2 on 16.03.2025.
//

#ifndef IMAGESTACKER_OPENCL_MANAGER_H
#define IMAGESTACKER_OPENCL_MANAGER_H

#define CL_TARGET_OPENCL_VERSION 200

#include <cstdint>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "CL/cl.h"

namespace OpenCL {
    extern cl_context context;
    extern cl_command_queue commandQueue;
    extern cl_program program;
    extern size_t maxGroupSize;

    void initCL(AAssetManager* gAssetManager);
    void releaseResources();
    cl_mem createBuffer(void* data, size_t size);
    cl_kernel createKernel(const char* name);
    void enqueueNDRangeKernel(cl_kernel kernel, cl_uint ND, size_t* offset, size_t* global, size_t* local);
    void readBuffer(cl_mem cl_buffer, void* buffer, size_t bufferSize);
    cl_mem cloneBuffer(cl_mem src_buffer, size_t size);
}

#endif //IMAGESTACKER_OPENCL_MANAGER_H
