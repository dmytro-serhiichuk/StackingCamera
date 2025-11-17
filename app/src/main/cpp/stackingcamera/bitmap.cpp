//
// Created by sedv2 on 15.01.2025.
//

#include "bitmap.h"

Bitmap::Bitmap(int32_t w, int32_t h, uint16_t *b, ColorType ct) {
    width = w;
    height = h;
    buffer = b;
    colorType = ct;

    bufferLength = colorType == ColorType::RGB ? w * h * 3 : w * h;
}

Bitmap::~Bitmap() {
    delete [] buffer;
    buffer = nullptr;
}

cl_mem Bitmap::createCLBuffer() {
    return OpenCL::createBuffer(buffer, bufferLength * sizeof(uint16_t));
}

Bitmap8* Bitmap::toGray8(cl_mem &inputBuffer) {
    if (colorType == ColorType::Grayscale) {
        return nullptr;
    }

    size_t outputDataLength = width * height;
    uint8_t* outputData = new uint8_t[outputDataLength] { 0 };

    cl_mem outputBuffer = OpenCL::createBuffer(outputData, outputDataLength);

    cl_kernel kernel = OpenCL::createKernel("to_grayscale_8");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 2, sizeof(int32_t), &width);

    size_t* global = new size_t[2] { (size_t)width, (size_t)height };
    OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, nullptr);
    OpenCL::readBuffer(outputBuffer, outputData, outputDataLength);

    delete[] global;

    clReleaseKernel(kernel);
    clReleaseMemObject(inputBuffer);

    inputBuffer = outputBuffer;
    return new Bitmap8(width, height, outputData);
}