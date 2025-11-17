//
// Created by sedv2 on 17.01.2025.
//

#include "warp.h"
#include "core.h"

Warper::Warper(Bitmap *bitmap) {
    outputWidth = bitmap->width;
    outputHeight = bitmap->height;
    outputBufferSize = bitmap->bufferLength;

    outputBuffer = OpenCL::createBuffer(bitmap->buffer, sizeof(uint16_t) * outputBufferSize);

    kernel = OpenCL::createKernel("warp_perspective");

    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);

    clSetKernelArg(kernel, 5, sizeof(int32_t), &outputWidth);
    clSetKernelArg(kernel, 6, sizeof(int32_t), &outputHeight);
}

Warper::~Warper() {
    clReleaseKernel(kernel);
    clReleaseMemObject(outputBuffer);
}

cl_mem initHBuffer(const Eigen::Matrix3d &H) {
    Eigen::Matrix3d invH = H.transpose().inverse();

    float *HData = new float[9];
    for (size_t i = 0; i < 9; i++) HData[i] = (float)invH.data()[i];

    cl_mem HBuffer = OpenCL::createBuffer(HData, sizeof(float) * 9);

    delete [] HData;
    return HBuffer;
}

BitmapPointer *Warper::warpPerspective(Bitmap *bitmap, const Eigen::Matrix3d &H) {
    cl_mem HBuffer = initHBuffer(H);

    cl_mem inputBuffer = OpenCL::createBuffer(bitmap->buffer, sizeof(uint16_t) * bitmap->bufferLength);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);

    clSetKernelArg(kernel, 2, sizeof(cl_mem), &HBuffer);
    clSetKernelArg(kernel, 3, sizeof(int32_t), &(bitmap->width));
    clSetKernelArg(kernel, 4, sizeof(int32_t), &(bitmap->height));

    uint16_t *outputData = new uint16_t[outputBufferSize] { 0 };

    size_t* global = new size_t [2] { (size_t)outputWidth, (size_t)outputHeight };
    OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, nullptr);
    OpenCL::readBuffer(outputBuffer, outputData, sizeof(uint16_t) * outputBufferSize);

    delete [] global;
    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(HBuffer);

    return new BitmapPointer {
            outputWidth,
            outputHeight,
            outputData
    };
}
