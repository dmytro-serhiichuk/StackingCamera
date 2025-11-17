//
// Created by sedv2 on 15.01.2025.
//

#include "fast.h"
#include "core.h"

namespace FAST {
    void
    FAST::detect(Bitmap8 &bitmap, cl_mem &imBuffer, Buffer<KeyPoint> &keyPoints, cl_mem &kpsBuffer,
                 uint32_t padding, uint32_t octave, float scaleFactor
    ) {
        int32_t kpCount = 0;
        cl_mem kpCountBuffer = OpenCL::createBuffer(&kpCount, sizeof(int32_t));

        cl_kernel kernel = OpenCL::createKernel("fast_9");

        clSetKernelArg(kernel, 0, sizeof(cl_mem), &imBuffer);
        clSetKernelArg(kernel, 1, sizeof(int32_t), &bitmap.width);
        clSetKernelArg(kernel, 2, sizeof(int32_t), &bitmap.height);
        clSetKernelArg(kernel, 3, sizeof(int32_t), &Core::FAST_THRESHOLD);
        clSetKernelArg(kernel, 4, sizeof(cl_mem), &kpsBuffer);
        clSetKernelArg(kernel, 5, sizeof(cl_mem), &kpCountBuffer);
        clSetKernelArg(kernel, 6, sizeof(uint32_t), &octave);
        clSetKernelArg(kernel, 7, sizeof(float), &scaleFactor);

        size_t* offsets = new size_t[2] { padding, padding };
        size_t* global = new size_t[2] { bitmap.width - padding * 2, bitmap.height - padding * 2 };
        OpenCL::enqueueNDRangeKernel(kernel, 2, offsets, global, nullptr);
        OpenCL::readBuffer(kpCountBuffer, &kpCount, sizeof(int32_t));
        OpenCL::readBuffer(kpsBuffer, keyPoints.end(), kpCount * sizeof(KeyPoint));
        keyPoints.size += kpCount;

        delete[] offsets;
        delete[] global;

        clReleaseKernel(kernel);
        clReleaseMemObject(kpCountBuffer);
    }
}
