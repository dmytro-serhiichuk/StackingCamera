//
// Created by sedv2 on 13.01.2026.
//

#include "warping.h"


WarpManager::WarpManager(BitmapPtr &baseBitmap) {
    outputWidth = baseBitmap.width;
    outputHeight = baseBitmap.height;
    outputBufferLength = baseBitmap.width * baseBitmap.height * (size_t)baseBitmap.colorSpace;
    outputBufferSize = outputBufferLength * (size_t)baseBitmap.depth;

    outputBuffer = CL::createBuffer(CL_MEM_WRITE_ONLY, outputBufferSize, nullptr);

    const char* kernelName = baseBitmap.depth == Depth::U16 ? "warp_perspective_16" : "warp_perspective_8";
    kernel = CL::createKernel(kernelName);

    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);

    clSetKernelArg(kernel, 5, sizeof(int32_t), &outputWidth);
    clSetKernelArg(kernel, 6, sizeof(int32_t), &outputHeight);
    int32_t channels = (int32_t)baseBitmap.colorSpace;
    clSetKernelArg(kernel, 7, sizeof(int32_t), &channels);
}

WarpManager::~WarpManager() {
    clReleaseKernel(kernel);
    clReleaseMemObject(outputBuffer);
}

static inline cl_mem initHBuffer(const Eigen::Matrix3d &H) {
    Eigen::Matrix3d invH = H.transpose().inverse();

    double *src = invH.data();
    float *dst = new float[9];
    for (size_t i = 0; i < 9; i++) dst[i] = (float)src[i];

    cl_mem buffer = CL::createBuffer(
        CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS,
        sizeof(float) * 9, dst
    );

    delete [] dst;
    return buffer;
}

BitmapPtr *WarpManager::warpSingleBitmap(Bitmap &bitmap, const Eigen::Matrix3d &H) {
    cl_mem HBuffer = initHBuffer(H);

    cl_mem inputBuffer = CL::createBuffer(
        CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS,
        bitmap.sizeOfBuffer(), bitmap.buffer
    );

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);

    clSetKernelArg(kernel, 2, sizeof(cl_mem), &HBuffer);
    clSetKernelArg(kernel, 3, sizeof(int32_t), &(bitmap.width));
    clSetKernelArg(kernel, 4, sizeof(int32_t), &(bitmap.height));

    uint8_t* outputBitmapBuffer = new uint8_t[outputBufferSize]();

    cl_event warpingFinished;

    size_t global[2] { (size_t)outputWidth, (size_t)outputHeight };
    clEnqueueNDRangeKernel(
        CL::queue, kernel, 2, nullptr,
        global, nullptr, 0, nullptr, &warpingFinished
    );
    clEnqueueReadBuffer(
        CL::queue, outputBuffer,
        CL_TRUE, 0, outputBufferSize, outputBitmapBuffer,
        1, &warpingFinished,nullptr
    );

    clReleaseEvent(warpingFinished);
    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(HBuffer);

    return new BitmapPtr(outputWidth, outputHeight, outputBitmapBuffer, bitmap.colorSpace, bitmap.depth);
}

List<BitmapPtr> *WarpManager::warp(List<Core::Data> &sources, uint32_t bestIndex,
                                   Buffer<Eigen::Matrix3d> &matrices) {
    auto warpedBitmaps = new List<BitmapPtr>(matrices.size);

    size_t matrixIndex = 0;
    for (size_t i = 0; i < sources.size; i++) {
        if (i == bestIndex) continue;
        JNIHelper::getInstance()->writeMessageToLog(false, "Starting warping image %zd", i);

        auto bitmap = sources.buffer[i]->bitmapPtr->read();
        warpedBitmaps->add(warpSingleBitmap(*bitmap, matrices[matrixIndex]));
        delete bitmap;
        matrixIndex++;
        JNIHelper::getInstance()->writeMessageToLog(false, "Image %zd warping completed\n", i);
    }

    return warpedBitmaps;
}
