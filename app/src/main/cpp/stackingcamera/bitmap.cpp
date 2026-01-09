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
    cl_mem b = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
            bufferLength * sizeof(uint16_t), buffer, nullptr
    );
    return b;
}

Bitmap8* Bitmap::toGray8(cl_mem &inputBuffer) {
    if (colorType == ColorType::Grayscale) {
        return nullptr;
    }

    size_t outputDataLength = width * height;

    cl_mem outputBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,
            outputDataLength, nullptr, nullptr
    );

    cl_kernel kernel = OpenCL::createKernel("to_grayscale_8");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 2, sizeof(int32_t), &width);

    size_t global [2] { (size_t)width, (size_t)height };
    OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, nullptr);
    clFinish(OpenCL::commandQueue);

    clReleaseKernel(kernel);
    clReleaseMemObject(inputBuffer);

    inputBuffer = outputBuffer;
    return new Bitmap8(width, height, nullptr);
}

Bitmap8 *Bitmap::toGray8WithBufferReading(cl_mem &inputBuffer) {
    if (colorType == ColorType::Grayscale) {
        return nullptr;
    }

    size_t outputDataLength = width * height;
    uint8_t* outputData = new uint8_t[outputDataLength]();

    cl_mem outputBuffer = OpenCL::createBuffer(outputData, outputDataLength);

    cl_kernel kernel = OpenCL::createKernel("to_grayscale_8");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 2, sizeof(int32_t), &width);

    size_t global[2] { (size_t)width, (size_t)height };
    OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, nullptr);
    OpenCL::readBuffer(outputBuffer, outputData, outputDataLength);

    clReleaseKernel(kernel);
    clReleaseMemObject(inputBuffer);

    inputBuffer = outputBuffer;
    return new Bitmap8(width, height, outputData);
}
