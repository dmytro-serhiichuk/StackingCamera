//
// Created by sedv2 on 16.03.2025.
//

#include "opencl-manager.h"
#include <stdexcept>
#include <string>
#include <android/log.h>

namespace OpenCL {
    cl_context context = nullptr;
    cl_command_queue commandQueue = nullptr;
    cl_program program = nullptr;
    size_t maxGroupSize = 0;

    char* readProgramSource(AAssetManager *assetManager) {
        AAsset* asset = AAssetManager_open(assetManager, "program.cl", AASSET_MODE_BUFFER);
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

    void initCL(AAssetManager* gAssetManager) {
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
        OpenCL::context = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot create the context");

        // Step 7: Creating command queue
        OpenCL::commandQueue = clCreateCommandQueue(OpenCL::context, device, 0, &status);
        if (status != CL_SUCCESS) throw std::runtime_error("Cannot create the command queue");

        const char* source = readProgramSource(gAssetManager);

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
            clGetProgramBuildInfo(OpenCL::program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log, nullptr);

            throw std::runtime_error(build_log);
        }

        status = clGetDeviceInfo(
                device,
                CL_DEVICE_MAX_WORK_GROUP_SIZE,
                sizeof(size_t),
                &maxGroupSize,
                nullptr
        );
    }
    void releaseResources() {
        clReleaseProgram(program);
        clReleaseCommandQueue(commandQueue);
        clReleaseContext(context);
    }
    cl_mem createBuffer(void *data, size_t size)
    {
        cl_int status;
        auto flags = data == nullptr ? CL_MEM_READ_WRITE : CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
        cl_mem buffer = clCreateBuffer(OpenCL::context, flags, size, data, &status);
        if (status != CL_SUCCESS) {
            __android_log_print(ANDROID_LOG_INFO, "__NATIVE__", "CL creating buffer error: %d", status);
            throw std::runtime_error("clbuffer creating error: " + std::to_string(status));
        }
        return buffer;
    }
    cl_kernel createKernel(const char *name)
    {
        cl_int status;
        cl_kernel kernel = clCreateKernel(program, name, &status);
        if (status != CL_SUCCESS) {
            __android_log_print(ANDROID_LOG_INFO, "__NATIVE__", "CL creating kernel error: %d", status);
            throw std::runtime_error("kernel creating error: " + std::to_string(status));
        }
        return kernel;
    }
    void enqueueNDRangeKernel(cl_kernel kernel, cl_uint ND, size_t* offset, size_t* global, size_t* local)
    {
        cl_int status = clEnqueueNDRangeKernel(
                commandQueue, kernel, ND, offset, global, local, 0, nullptr, nullptr
        );
        if (status != CL_SUCCESS) {
            __android_log_print(ANDROID_LOG_INFO, "__NATIVE__", "CL EnqueueNDRangeKernel failed: %d", status);
            throw std::runtime_error("EnqueueNDRangeKernel failed: " + std::to_string(status));
        }
    }
    void readBuffer(cl_mem cl_buffer, void *buffer, size_t bufferSize)
    {
        cl_int status = clEnqueueReadBuffer(
                commandQueue, cl_buffer, CL_TRUE, 0, bufferSize, buffer, 0, nullptr, nullptr
        );
        if (status != CL_SUCCESS) {
            __android_log_print(ANDROID_LOG_INFO, "__NATIVE__", "CL EnqueueReadBuffer failed: %d", status);
            throw std::runtime_error("EnqueueReadBuffer failed: " + std::to_string(status));
        }
    }

    cl_mem cloneBuffer(cl_mem src_buffer, size_t size) {
        cl_mem dst_buffer = clCreateBuffer(
                context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
                size, nullptr, nullptr
        );
        cl_int status = clEnqueueCopyBuffer(commandQueue, src_buffer, dst_buffer, 0, 0, size, 0, nullptr, nullptr);
        if (status != CL_SUCCESS) {
            __android_log_print(ANDROID_LOG_INFO, "__NATIVE__", "CL CopyBuffer failed: %d", status);
            throw std::runtime_error("CopyBuffer failed: " + std::to_string(status));
        }
        clFinish(commandQueue);
        return dst_buffer;
    }
}