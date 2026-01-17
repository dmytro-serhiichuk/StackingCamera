//
// Created by sedv2 on 11.01.2026.
//

#ifndef STACKINGCAMERA_OPENCL_H
#define STACKINGCAMERA_OPENCL_H

#define CL_TARGET_OPENCL_VERSION 200

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include "CL/cl.h"

namespace CL {
    typedef struct ImageChannelTypeSupportInfo {
        bool SUPPORT_READ_ONLY = false;
        bool SUPPORT_WRITE_ONLY = false;
        bool SUPPORT_READ_WRITE = false;
    } ImageChannelTypeSupportInfo;

    typedef struct ImageChannelOrderSupportInfo {
        cl_channel_order order;
        ImageChannelTypeSupportInfo UNORM_INT8_SUPPORT;
        ImageChannelTypeSupportInfo UNORM_INT16_SUPPORT;
    } ImageChannelOrderSupportInfo;

    extern cl_context context;
    extern cl_command_queue queue;
    extern cl_program program;
    extern size_t maxGroupSize;

    extern ImageChannelOrderSupportInfo grayscaleInfo;
    extern ImageChannelOrderSupportInfo rgbInfo;
    extern ImageChannelOrderSupportInfo rgbaInfo;

    void init(AAssetManager* aam);
    cl_mem createBuffer(cl_mem_flags flags, size_t size, void* data);
    cl_kernel createKernel(const char* name);
    void enqueueNDRangeKernel(cl_kernel kernel, cl_uint ND, size_t* offset, size_t* global, size_t* local);
    void readBuffer(cl_mem cl_buffer, void* buffer, size_t bufferSize, cl_bool block);
    void copyBuffer(cl_mem src, cl_mem dst, size_t size);
}

#endif //STACKINGCAMERA_OPENCL_H
