//
// Created by sedv2 on 12.01.2026.
//

#include <cmath>
#include "fast.h"
#include "../opencl.h"
#include "../core.h"

namespace FAST {
    FAST_Buffers::FAST_Buffers(size_t size) {
        kpsBuffer = CL::createBuffer(
                CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                size, nullptr
        );
        counterBuffer = CL::createBuffer(
                CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                sizeof(int32_t), nullptr
        );
    }

    FAST_Buffers::~FAST_Buffers() {
        clReleaseMemObject(kpsBuffer);
        clReleaseMemObject(counterBuffer);
    }

    static size_t calcGlobalSize(size_t startSize, size_t devider) {
        size_t remainder = startSize % devider;
        if (remainder == 0) {
            return startSize;
        } else {
            return startSize + devider - remainder;
        }
    }

    void detect(BitmapInfo &bitmap, cl_mem &imageBuffer, Buffer<KeyPoint> &keyPoints,
                FAST_Buffers &fastBuffers, uint32_t padding, uint32_t octave, float scaleFactor) {
        cl_event counterFilled, fastFinished, counterRead;

        int32_t count = 0;
        clEnqueueFillBuffer(
            CL::computeQueue, fastBuffers.counterBuffer,
            &count,sizeof(int32_t), 0, sizeof(int32_t),
            0,nullptr, &counterFilled
        );

        cl_kernel kernel = CL::createKernel("fast_9");

        size_t localWidth = 1;
        size_t localHeight = CL::maxGroupSize;

        for (size_t i = floor(sqrt(CL::maxGroupSize)); i >= 1; --i) {
            if (CL::maxGroupSize % i == 0) {
                localWidth = i;
                localHeight = CL::maxGroupSize / i;
                break;
            }
        }

        size_t gw = calcGlobalSize(bitmap.width, localWidth);
        size_t gh = calcGlobalSize(bitmap.height, localHeight);
        size_t localSize = (localWidth + 6) * (localHeight + 6);

        clSetKernelArg(kernel, 0, sizeof(cl_mem), &imageBuffer);
        clSetKernelArg(kernel, 1, sizeof(int32_t), &bitmap.width);
        clSetKernelArg(kernel, 2, sizeof(int32_t), &bitmap.height);
        clSetKernelArg(kernel, 3, sizeof(int32_t), &Core::FAST_THRESHOLD);
        clSetKernelArg(kernel, 4, sizeof(cl_mem), &fastBuffers.kpsBuffer);
        clSetKernelArg(kernel, 5, sizeof(cl_mem), &fastBuffers.counterBuffer);
        clSetKernelArg(kernel, 6, sizeof(uint32_t), &octave);
        clSetKernelArg(kernel, 7, sizeof(float), &scaleFactor);
        clSetKernelArg(kernel, 8, sizeof(uint32_t), &padding);
        clSetKernelArg(kernel, 9, localSize, nullptr);

        size_t local[2] { localWidth, localHeight };
        size_t global[2] { gw, gh };
        clEnqueueNDRangeKernel(
            CL::computeQueue, kernel, 2, nullptr,
            global, local, 1, &counterFilled, &fastFinished
        );
        clEnqueueReadBuffer(
            CL::transferQueue, fastBuffers.counterBuffer,
            CL_FALSE, 0, sizeof(int32_t), &count,
            1, &fastFinished, &counterRead
        );
        clEnqueueReadBuffer(
            CL::transferQueue, fastBuffers.kpsBuffer,
            CL_TRUE, 0, count * sizeof(KeyPoint), keyPoints.end(),
            1, &counterRead,nullptr
        );

        keyPoints.size += count;

        clReleaseEvent(counterFilled);
        clReleaseEvent(fastFinished);
        clReleaseEvent(counterRead);
        clReleaseKernel(kernel);
    }
}


