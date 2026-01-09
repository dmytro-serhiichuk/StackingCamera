//
// Created by sedv2 on 15.01.2025.
//

#include "fast.h"
#include "core.h"

namespace FAST {
    static size_t calcGlobalSize(size_t startSize, size_t devider) {
        size_t remainder = startSize % devider;
        if (remainder == 0) {
            return startSize;
        } else {
            return startSize + devider - remainder;
        }
    }

    void
    FAST::detect(Bitmap8 &bitmap, cl_mem &imBuffer, Buffer<KeyPoint> &keyPoints, cl_mem &kpsBuffer,
                 cl_mem &kpsCounterBuffer, uint32_t padding, uint32_t octave, float scaleFactor
    ) {
        int32_t kpCount = 0;
        clEnqueueWriteBuffer(OpenCL::commandQueue, kpsCounterBuffer,
                             CL_TRUE, 0, sizeof(int32_t),
                             &kpCount, 0,nullptr, nullptr);

        cl_kernel kernel = OpenCL::createKernel("fast_9");

        size_t localWidth = 1;
        size_t localHeight = OpenCL::maxGroupSize;

        for (size_t i = floor(sqrt(OpenCL::maxGroupSize)); i >= 1; --i) {
            if (OpenCL::maxGroupSize % i == 0) {
                localWidth = i;
                localHeight = OpenCL::maxGroupSize / i;
                break;
            }
        }

        size_t gw = calcGlobalSize(bitmap.width, localWidth);
        size_t gh = calcGlobalSize(bitmap.height, localHeight);
        size_t localSize = (localWidth + 6) * (localHeight + 6);

        clSetKernelArg(kernel, 0, sizeof(cl_mem), &imBuffer);
        clSetKernelArg(kernel, 1, sizeof(int32_t), &bitmap.width);
        clSetKernelArg(kernel, 2, sizeof(int32_t), &bitmap.height);
        clSetKernelArg(kernel, 3, sizeof(int32_t), &Core::FAST_THRESHOLD);
        clSetKernelArg(kernel, 4, sizeof(cl_mem), &kpsBuffer);
        clSetKernelArg(kernel, 5, sizeof(cl_mem), &kpsCounterBuffer);
        clSetKernelArg(kernel, 6, sizeof(uint32_t), &octave);
        clSetKernelArg(kernel, 7, sizeof(float), &scaleFactor);
        clSetKernelArg(kernel, 8, sizeof(uint32_t), &padding);
        clSetKernelArg(kernel, 9, localSize, nullptr);

        size_t local[2] { localWidth, localHeight };
        size_t global[2] { gw, gh };
        OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, local);
        OpenCL::readBuffer(kpsCounterBuffer, &kpCount, sizeof(int32_t));
        OpenCL::readBuffer(kpsBuffer, keyPoints.end(), kpCount * sizeof(KeyPoint));
        keyPoints.size += kpCount;

        clReleaseKernel(kernel);
    }
}
