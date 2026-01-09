//
// Created by sedv2 on 16.03.2025.
//

#include "bitmap-8.h"

Bitmap8::Bitmap8(int32_t _width, int32_t _height, uint8_t *_buffer) {
    width = _width;
    height = _height;
    buffer = _buffer;
    bufferSize = width * height * sizeof(uint8_t);
}
Bitmap8::~Bitmap8() {
    delete[] buffer;
}

void Bitmap8::CLAHE(cl_mem &inputBuffer, uint32_t tileCount, float fClipLimit) {
    const uint32_t BINS_COUNT = 256;

    const uint32_t tileWidth = width / tileCount;
    const uint32_t tileHeight = height / tileCount;

    uint64_t clipLimit = (uint64_t)(fClipLimit * (tileWidth * tileHeight) / BINS_COUNT);
    size_t mapLength = tileCount * tileCount * BINS_COUNT;
    size_t mapSize = mapLength * sizeof(uint64_t);

    cl_mem mapBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,
            mapSize, nullptr, nullptr
    );
    cl_int zero = 0;
    clEnqueueFillBuffer(OpenCL::commandQueue, mapBuffer, &zero, sizeof(zero),
                        0, mapSize, 0, nullptr,nullptr);

    cl_kernel kernel = OpenCL::createKernel("clahe_make_lut");

    cl_int status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    status = clSetKernelArg(kernel, 1, sizeof(uint32_t), &width);
    status = clSetKernelArg(kernel, 2, sizeof(uint32_t), &height);
    status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &mapBuffer);
    status = clSetKernelArg(kernel, 4, sizeof(uint32_t), &tileCount);
    status = clSetKernelArg(kernel, 5, sizeof(uint64_t), &clipLimit);
    status = clSetKernelArg(kernel, 6, sizeof(uint32_t), &tileWidth);
    status = clSetKernelArg(kernel, 7, sizeof(uint32_t), &tileHeight);

    size_t global[2] { tileCount, tileCount };
    OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, nullptr);
    clFinish(OpenCL::commandQueue);

    clReleaseKernel(kernel);

    kernel = OpenCL::createKernel("clahe_interpolate");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(uint32_t), &width);
    clSetKernelArg(kernel, 2, sizeof(uint32_t), &height);
    clSetKernelArg(kernel, 3, sizeof(cl_mem), &mapBuffer);
    clSetKernelArg(kernel, 4, sizeof(uint32_t), &tileCount);
    clSetKernelArg(kernel, 5, sizeof(uint32_t), &tileWidth);
    clSetKernelArg(kernel, 6, sizeof(uint32_t), &tileHeight);

    global[0] = width;
    global[1] = height;
    OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, nullptr);
    clFinish(OpenCL::commandQueue);

    clReleaseKernel(kernel);
    clReleaseMemObject(mapBuffer);
}

void Bitmap8::blur(cl_mem &inputBuffer, GaussianKernel &gk) {
    cl_mem outputBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,
            bufferSize, nullptr, nullptr
    );
    cl_mem gaussianBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
            gk.sizeOf, gk.buffer, nullptr
    );

    cl_kernel kernel = OpenCL::createKernel("gaussian_blur");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 2, sizeof(int32_t), &width);
    clSetKernelArg(kernel, 3, sizeof(int32_t), &height);
    clSetKernelArg(kernel, 4, sizeof(cl_mem), &gaussianBuffer);
    clSetKernelArg(kernel, 5, sizeof(int32_t), &gk.radius);

    size_t global[2] { (size_t)width, (size_t)height };
    OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, nullptr);
    clFinish(OpenCL::commandQueue);

    clReleaseKernel(kernel);
    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(gaussianBuffer);

    inputBuffer = outputBuffer;
}

bool Bitmap8::canResize(float scaleFactor, uint32_t minSize) {
    return width * scaleFactor > minSize && height * scaleFactor > minSize;
}

void Bitmap8::resize(cl_mem &inputBuffer, float scaleFactor) {
    int32_t rWidth = width * scaleFactor;
    int32_t rHeight = height * scaleFactor;

    size_t rBufferLength = rWidth * rHeight;

    cl_mem outputBuffer = clCreateBuffer(
            OpenCL::context, CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,
            rBufferLength, nullptr, nullptr
    );

    cl_kernel kernel = OpenCL::createKernel("resize");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 2, sizeof(int32_t), &width);
    clSetKernelArg(kernel, 3, sizeof(int32_t), &height);
    clSetKernelArg(kernel, 4, sizeof(int32_t), &rWidth);
    clSetKernelArg(kernel, 5, sizeof(float), &scaleFactor);

    size_t global[2] { (size_t)rWidth, (size_t)rHeight };
    OpenCL::enqueueNDRangeKernel(kernel, 2, nullptr, global, nullptr);
    clFinish(OpenCL::commandQueue);

    clReleaseKernel(kernel);
    clReleaseMemObject(inputBuffer);

    inputBuffer = outputBuffer;

    delete [] buffer;
    width = rWidth;
    height = rHeight;
    bufferSize = rBufferLength;
}

int32_t *Bitmap8::integral() {
    int32_t i_w = width + 1;
    int32_t i_h = height + 1;
    int32_t* i_buffer = new int32_t[i_w * i_h] { 0 };

    for (size_t y = 1; y < i_h; y++) {
        for (size_t x = 1; x < i_w; x++) {
            i_buffer[y * i_w + x] = buffer[(y-1)*width+x-1] +
                                    i_buffer[(y-1)*i_w+x] +
                                    i_buffer[y*i_w+x-1] -
                                    i_buffer[(y-1)*i_w+x-1];
        }
    }
    return i_buffer;
}