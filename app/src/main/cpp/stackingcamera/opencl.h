//
// Created by sedv2 on 11.01.2026.
//

#ifndef STACKINGCAMERA_OPENCL_H
#define STACKINGCAMERA_OPENCL_H

#define CL_TARGET_OPENCL_VERSION 200

#include "CL/cl.h"

namespace CL {
    extern cl_context context;
    extern cl_command_queue queue;
    extern cl_program program;
    extern size_t maxGroupSize;

    void init(const char *programSrc);
    cl_mem createBuffer(cl_mem_flags flags, size_t size, void* data);
    cl_kernel createKernel(const char* name);
    void enqueueNDRangeKernel(cl_kernel kernel, cl_uint ND, size_t* offset, size_t* global, size_t* local);
    void readBuffer(cl_mem cl_buffer, void* buffer, size_t bufferSize, cl_bool block);
    void copyBuffer(cl_mem src, cl_mem dst, size_t size);
}

#endif //STACKINGCAMERA_OPENCL_H
