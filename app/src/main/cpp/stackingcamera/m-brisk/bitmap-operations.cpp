//
// Created by sedv2 on 12.01.2026.
//

#include "bitmap-operations.h"
#include <cmath>

GaussianKernel::GaussianKernel(float *_buffer, int32_t _radius, size_t _sizeof) {
    buffer = _buffer;
    radius = _radius;
    sizeOf = _sizeof;
}

GaussianKernel::~GaussianKernel() {
    delete [] buffer;
}

GaussianKernel* GaussianKernel::create(float scaleFactor) {
    float sigma = BASE_SIGMA * scaleFactor;

    auto radius = (int32_t)ceil(3 * sigma);
    int32_t diameter = 2 * radius + 1;
    auto buffer = new float[diameter];

    float sum = 0.0f;

    for (int i = -radius; i <= radius; i++) {
        float value = exp(-(i * i) / (2 * sigma * sigma));

        buffer[i + radius] = value;
        sum += value;
    }

    for (size_t i = 0; i < diameter; i++) {
        buffer[i] /= sum;
    }

    return new GaussianKernel(buffer, radius, diameter * sizeof(float));
}

void BitmapInfo::update() {
    auto spp = getSamplesPerPixel(colorModel);
    bufferLength = width * height * spp;
    stride = width * spp;
}

void toGray8(BitmapInfo &bitmap, cl_mem &inputBuffer) {
    size_t outputDataLength = bitmap.width * bitmap.height;

    cl_mem outputBuffer = CL::createBuffer(
            CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,
            outputDataLength, nullptr
    );

    const char* kernelName = bitmap.depth == Depth::U16 ? "from_rgb16_to_gray8" : "from_rgb8_to_gray8";
    cl_kernel kernel = CL::createKernel(kernelName);

    int32_t channels = getSamplesPerPixel(bitmap.colorModel);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 2, sizeof(uint32_t), &bitmap.width);
    clSetKernelArg(kernel, 3, sizeof(int32_t), &channels);

    size_t global [2] { (size_t)bitmap.width, (size_t)bitmap.height };
    clEnqueueNDRangeKernel(
            CL::queue, kernel, 2, nullptr, global,
            nullptr, 0, nullptr, nullptr
    );
    clFinish(CL::queue);

    clReleaseKernel(kernel);
    clReleaseMemObject(inputBuffer);

    inputBuffer = outputBuffer;

    bitmap.depth = Depth::U8;
    bitmap.colorModel = ColorModel::GRAY;
    bitmap.update();
}

Bitmap toGray8WithReading(const Bitmap &bitmap, cl_mem &inputBuffer) {
    size_t outputDataLength = bitmap.width * bitmap.height;

    auto outputBitmapBuffer = new uint8_t[outputDataLength];
    cl_mem outputBuffer = CL::createBuffer(
            CL_MEM_READ_WRITE,
            outputDataLength, nullptr
    );

    const char* kernelName = bitmap.depth == Depth::U16 ? "from_rgb16_to_gray8" : "from_rgb8_to_gray8";
    cl_kernel kernel = CL::createKernel(kernelName);

    int32_t channels = getSamplesPerPixel(bitmap.colorModel);

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 2, sizeof(uint32_t), &bitmap.width);
    clSetKernelArg(kernel, 3, sizeof(int32_t), &channels);

    cl_event finished;

    size_t global [2] { (size_t)bitmap.width, (size_t)bitmap.height };
    clEnqueueNDRangeKernel(
            CL::queue, kernel, 2, nullptr, global,
            nullptr, 0, nullptr, &finished
    );
    clEnqueueReadBuffer(
        CL::queue, outputBuffer,
        CL_TRUE, 0,outputDataLength, outputBitmapBuffer,
        1, &finished ,nullptr
    );

    clReleaseEvent(finished);
    clReleaseKernel(kernel);
    clReleaseMemObject(inputBuffer);

    inputBuffer = outputBuffer;

    return Bitmap {bitmap.width, bitmap.height, outputBitmapBuffer, Depth::U8, ColorModel::GRAY, ColorSpace::Other};
}

void CLAHE(BitmapInfo &bitmap, cl_mem &inputBuffer, uint32_t tileCount, float fClipLimit) {
    const uint32_t BINS_COUNT = 256;

    const uint32_t tileWidth = bitmap.width / tileCount;
    const uint32_t tileHeight = bitmap.height / tileCount;

    auto clipLimit = (uint64_t)(fClipLimit * (float)(tileWidth * tileHeight) / BINS_COUNT);
    size_t mapLength = tileCount * tileCount * BINS_COUNT;
    size_t mapSize = mapLength * sizeof(uint32_t);

    cl_mem mapBuffer = CL::createBuffer(CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,mapSize, nullptr);

    cl_event lutCreatedEvent;

    cl_kernel make_lut_kernel = CL::createKernel("clahe_make_lut");

    clSetKernelArg(make_lut_kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(make_lut_kernel, 1, sizeof(uint32_t), &bitmap.width);
    clSetKernelArg(make_lut_kernel, 2, sizeof(uint32_t), &bitmap.height);
    clSetKernelArg(make_lut_kernel, 3, sizeof(cl_mem), &mapBuffer);
    clSetKernelArg(make_lut_kernel, 4, sizeof(uint32_t), &tileCount);
    clSetKernelArg(make_lut_kernel, 5, sizeof(uint64_t), &clipLimit);
    clSetKernelArg(make_lut_kernel, 6, sizeof(uint32_t), &tileWidth);
    clSetKernelArg(make_lut_kernel, 7, sizeof(uint32_t), &tileHeight);

    size_t local[1] { CL::maxGroupSize };
    size_t global1[1] { tileCount * CL::maxGroupSize * tileCount };
    clEnqueueNDRangeKernel(
        CL::queue, make_lut_kernel, 1, nullptr, global1,
        local, 0, nullptr, &lutCreatedEvent
    );

    cl_kernel interpolate_kernel = CL::createKernel("clahe_interpolate");

    clSetKernelArg(interpolate_kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(interpolate_kernel, 1, sizeof(uint32_t), &bitmap.width);
    clSetKernelArg(interpolate_kernel, 2, sizeof(uint32_t), &bitmap.height);
    clSetKernelArg(interpolate_kernel, 3, sizeof(cl_mem), &mapBuffer);
    clSetKernelArg(interpolate_kernel, 4, sizeof(uint32_t), &tileCount);
    clSetKernelArg(interpolate_kernel, 5, sizeof(uint32_t), &tileWidth);
    clSetKernelArg(interpolate_kernel, 6, sizeof(uint32_t), &tileHeight);

    clFinish(CL::queue);

    size_t global2[2] { bitmap.width, bitmap.height };
    clEnqueueNDRangeKernel(
            CL::queue, interpolate_kernel, 2, nullptr, global2,
            nullptr, 1, &lutCreatedEvent, nullptr
    );
    clFinish(CL::queue);

    clReleaseEvent(lutCreatedEvent);
    clReleaseKernel(make_lut_kernel);
    clReleaseKernel(interpolate_kernel);
    clReleaseMemObject(mapBuffer);
}

void blur(BitmapInfo &bitmap, cl_mem &inputBuffer, GaussianKernel &gk) {
    cl_mem tempBuffer = CL::createBuffer(
        CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,
        bitmap.sizeOfBuffer(), nullptr
    );
    cl_mem outputBuffer = CL::createBuffer(
        CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,
        bitmap.sizeOfBuffer(), nullptr
    );
    cl_mem gaussianBuffer = CL::createBuffer(
            CL_MEM_HOST_NO_ACCESS | CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
            gk.sizeOf, gk.buffer
    );

    size_t global[2] { (size_t)bitmap.width, (size_t)bitmap.height };

    cl_kernel kernel_h = CL::createKernel("gaussian_blur_horizontal");

    clSetKernelArg(kernel_h, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel_h, 1, sizeof(cl_mem), &tempBuffer);
    clSetKernelArg(kernel_h, 2, sizeof(int32_t), &bitmap.width);
    clSetKernelArg(kernel_h, 3, sizeof(int32_t), &bitmap.height);
    clSetKernelArg(kernel_h, 4, sizeof(cl_mem), &gaussianBuffer);
    clSetKernelArg(kernel_h, 5, sizeof(int32_t), &gk.radius);

    cl_event h_finished;

    clEnqueueNDRangeKernel(
            CL::queue, kernel_h, 2, nullptr, global,
            nullptr, 0, nullptr, &h_finished
    );

    cl_kernel kernel_v = CL::createKernel("gaussian_blur_vertical");

    clSetKernelArg(kernel_v, 0, sizeof(cl_mem), &tempBuffer);
    clSetKernelArg(kernel_v, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel_v, 2, sizeof(int32_t), &bitmap.width);
    clSetKernelArg(kernel_v, 3, sizeof(int32_t), &bitmap.height);
    clSetKernelArg(kernel_v, 4, sizeof(cl_mem), &gaussianBuffer);
    clSetKernelArg(kernel_v, 5, sizeof(int32_t), &gk.radius);

    clEnqueueNDRangeKernel(
            CL::queue, kernel_v, 2, nullptr, global,
            nullptr, 1, &h_finished, nullptr
    );
    clFinish(CL::queue);

    clReleaseEvent(h_finished);
    clReleaseKernel(kernel_h);
    clReleaseKernel(kernel_v);
    clReleaseMemObject(inputBuffer);
    clReleaseMemObject(tempBuffer);
    clReleaseMemObject(gaussianBuffer);

    inputBuffer = outputBuffer;
}

bool resize(BitmapInfo &bitmap, cl_mem &inputBuffer, float scaleFactor, uint32_t minSize) {
    bool canResize = bitmap.width * scaleFactor > minSize && bitmap.height * scaleFactor > minSize;
    if (!canResize) return false;

    int32_t rWidth = bitmap.width * scaleFactor;
    int32_t rHeight = bitmap.height * scaleFactor;

    size_t rBufferLength = rWidth * rHeight;

    cl_mem outputBuffer = CL::createBuffer(
            CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_WRITE,
            rBufferLength, nullptr
    );

    cl_kernel kernel = CL::createKernel("resize");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputBuffer);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &outputBuffer);
    clSetKernelArg(kernel, 2, sizeof(int32_t), &bitmap.width);
    clSetKernelArg(kernel, 3, sizeof(int32_t), &bitmap.height);
    clSetKernelArg(kernel, 4, sizeof(int32_t), &rWidth);
    clSetKernelArg(kernel, 5, sizeof(float), &scaleFactor);

    size_t global[2] { (size_t)rWidth, (size_t)rHeight };
    clEnqueueNDRangeKernel(
            CL::queue, kernel, 2, nullptr,
            global, nullptr, 0, nullptr, nullptr
    );
    clFinish(CL::queue);

    clReleaseKernel(kernel);
    clReleaseMemObject(inputBuffer);

    inputBuffer = outputBuffer;

    bitmap.width = rWidth;
    bitmap.height = rHeight;
    bitmap.update();

    return true;
}

int32_t *getIntegralImage(Bitmap &bitmap) {
    int32_t i_w = bitmap.width + 1;
    int32_t i_h = bitmap.height + 1;
    auto i_buffer = new int32_t[i_w * i_h]();

    for (size_t y = 1; y < i_h; y++) {
        for (size_t x = 1; x < i_w; x++) {
            i_buffer[y * i_w + x] = bitmap.buffer[(y-1)*bitmap.width+x-1] +
                                    i_buffer[(y-1)*i_w+x] +
                                    i_buffer[y*i_w+x-1] -
                                    i_buffer[(y-1)*i_w+x-1];
        }
    }
    return i_buffer;
}
