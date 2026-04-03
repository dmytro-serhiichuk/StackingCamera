//
// Created by sedv2 on 11.01.2026.
//

#include "opencl.h"
#include <stdexcept>

namespace CL {
    static char logBuffer[256];

    static char* readProgramSource(AAssetManager *aam) {
        AAsset* asset = AAssetManager_open(aam, "program.cl", AASSET_MODE_BUFFER);
        if (!asset) {
            throw std::runtime_error("Program source file opening failed");
        }
        size_t sourceSize = AAsset_getLength(asset);
        char* buffer = new char[sourceSize + 1];
        AAsset_read(asset, buffer, sourceSize);
        AAsset_close(asset);
        buffer[sourceSize] = '\0';
        return buffer;
    }

    cl_context context = nullptr;
    cl_command_queue queue = nullptr;
    cl_program program = nullptr;
    size_t maxGroupSize = 0;

    void init(AAssetManager *aam) {
        // Step 1.1: Getting platforms num
        cl_uint numPlatforms;
        cl_int status = clGetPlatformIDs(0, nullptr, &numPlatforms);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot get the number of platforms");

        // Step 1.2: Getting platforms
        cl_platform_id* platforms = new cl_platform_id[numPlatforms];
        status = clGetPlatformIDs(numPlatforms, platforms, nullptr);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot get platforms");

        // Step 2: Select first platform
        cl_platform_id platform = platforms[0];

        // Step 3.1: Getting devices num
        cl_uint numDevices;
        status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot get the number of devices");

        // Step 3.2: Getting devices
        cl_device_id* devices = new cl_device_id[numDevices];
        status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, numDevices, devices, nullptr);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot get devices");

        // Step 4: Select first device
        cl_device_id device = devices[0];

        // Step 5: Checking Version
        char version_p[128];
        clGetPlatformInfo(platform, CL_PLATFORM_VERSION, sizeof(version_p), version_p, nullptr);
        if (version_p[7] < 2) throw std::runtime_error("Invalid platform version");

        char version_d[128];
        clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(version_d), version_d, nullptr);
        if (version_d[7] < 2) throw std::runtime_error("Invalid device version");

        // Step 6: Creating context
        context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot create the context");

        // Step 7: Creating command queue
        cl_properties props[] = {
            CL_QUEUE_PROPERTIES,
            (cl_command_queue_properties)(CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE),
            0
        };
        queue = clCreateCommandQueueWithProperties(context, device, props, &status);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot create compute queue");

        const char* source = readProgramSource(aam);

        // Step 9: Create program
        program = clCreateProgramWithSource(context, 1, &source, nullptr, &status);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot create the program");
        delete [] source;

        // Step 10: Build program
        status = clBuildProgram(program, numDevices, devices, nullptr, nullptr, nullptr);
        if (status != CL_SUCCESS) {
            size_t log_size = 0;
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
            char* build_log = new char[log_size];
            clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log, nullptr);

            throw std::runtime_error(build_log);
        }

        // Step 11: Get max size of work group
        status = clGetDeviceInfo(
                device,
                CL_DEVICE_MAX_WORK_GROUP_SIZE,
                sizeof(size_t),
                &maxGroupSize,
                nullptr
        );
        if (status != CL_SUCCESS) {
            throw std::runtime_error("Cannot get max work group size");
        }
    }

    cl_mem createBuffer(cl_mem_flags flags, size_t size, void *data) {
        cl_int status;
        cl_mem buffer = clCreateBuffer(context, flags, size, data, &status);
        if (status != CL_SUCCESS) {
            printf(logBuffer, "buffer creating error: %d", status);
            throw std::runtime_error(logBuffer);
        }
        return buffer;
    }

    cl_kernel createKernel(const char *name) {
        cl_int status;
        cl_kernel kernel = clCreateKernel(program, name, &status);
        if (status != CL_SUCCESS) {
            sprintf(logBuffer, "kernel creating error: %d", status);
            throw std::runtime_error(logBuffer);
        }
        return kernel;
    }

    void enqueueNDRangeKernel(cl_kernel kernel, cl_uint ND, size_t* offset, size_t* global, size_t* local)
    {
        cl_int status = clEnqueueNDRangeKernel(
                queue, kernel, ND, offset, global, local, 0, nullptr, nullptr
        );
        if (status != CL_SUCCESS) {
            sprintf(logBuffer, "EnqueueNDRangeKernel error: %d", status);
            throw std::runtime_error(logBuffer);
        }
    }
    void readBuffer(cl_mem cl_buffer, void *buffer, size_t bufferSize, cl_bool block)
    {
        cl_int status = clEnqueueReadBuffer(
                queue, cl_buffer, block, 0, bufferSize, buffer, 0, nullptr, nullptr
        );
        if (status != CL_SUCCESS) {
            sprintf(logBuffer, "EnqueueReadBuffer error: %d", status);
            throw std::runtime_error(logBuffer);
        }
    }

    void copyBuffer(cl_mem src, cl_mem dst, size_t size) {
        cl_int status = clEnqueueCopyBuffer(queue, src, dst, 0, 0, size, 0, nullptr, nullptr);
        if (status != CL_SUCCESS) {
            sprintf(logBuffer, "CopyBuffer failed: %d", status);
            throw std::runtime_error(logBuffer);
        }
        clFinish(queue);
    }
};

