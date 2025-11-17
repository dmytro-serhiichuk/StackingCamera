//
// Created by sedv2 on 06.02.2025.
//

#include "matcher.h"
#include "core.h"

Match::Match() {
    index1 = 0;
    index2 = 0;
    distance = 0;
}

Match::Match(uint32_t _i1, uint32_t _i2, uint32_t _dist) {
    index1 = _i1;
    index2 = _i2;
    distance = _dist;
}


Buffer<Buffer<Match>> *
Matcher::match(BitmapPointer &bmp, List<Descriptors> &descriptors, Buffer<KeyPoint> &keyPoints,
               uint32_t bestIndex) {
    size_t matchesIndex = 0;
    Buffer<Buffer<Match>> *matches = new Buffer<Buffer<Match>>(descriptors.size - 1);
    matches->size = descriptors.size - 1;

    Descriptors& descriptors1 = *descriptors.buffer[bestIndex];

    cl_mem buffer1 = OpenCL::createBuffer(descriptors1.buffer, descriptors1.sizeOf());

    cl_kernel kernel = OpenCL::createKernel("find_closest_descriptors");

    clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer1);
    clSetKernelArg(kernel, 4, sizeof(int32_t), &descriptors1.count);

    size_t* global = new size_t[1] { descriptors1.count };

    uint32_t chunkWidth = bmp.width / Core::CHUNKS_PER_SIDE;
    uint32_t chunkHeight = bmp.height / Core::CHUNKS_PER_SIDE;

    for (size_t i = 0; i < descriptors.size; i++) {
        if (i == bestIndex) continue;

        Descriptors& descriptors2 = *descriptors.buffer[i];

        size_t o_count = descriptors1.count;
        size_t o_size = o_count * sizeof(int32_t);
        int32_t* closestIndices = new int32_t[descriptors1.count] { 0 };
        int32_t* distances = new int32_t[descriptors1.count] { 0 };

        cl_mem buffer2 = OpenCL::createBuffer(descriptors2.buffer, descriptors2.sizeOf());
        cl_mem closestIndicesBuffer = OpenCL::createBuffer(closestIndices, o_size);
        cl_mem distancesBuffer = OpenCL::createBuffer(distances, o_size);

        clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer2);
        clSetKernelArg(kernel, 2, sizeof(cl_mem), &closestIndicesBuffer);
        clSetKernelArg(kernel, 3, sizeof(cl_mem), &distancesBuffer);
        clSetKernelArg(kernel, 5, sizeof(int32_t), &descriptors2.count);

        OpenCL::enqueueNDRangeKernel(kernel, 1, nullptr, global, nullptr);
        OpenCL::readBuffer(closestIndicesBuffer, closestIndices, o_size);
        OpenCL::readBuffer(distancesBuffer, distances, o_size);

        Buffer<Match>* l_matches = &matches->buffer[matchesIndex];
        l_matches->buffer = new Match[o_count];
        l_matches->size = o_count;
        l_matches->capacity = o_count;

        size_t fi = 0;
        for (uint32_t j = 0; j < descriptors1.count; j++) {
            if (distances[j] < 30) {
                (*l_matches)[fi] = Match(closestIndices[j], j, distances[j]);
                fi++;
            }
        }
        l_matches->size = fi;

        delete[] closestIndices;
        delete[] distances;
        clReleaseMemObject(closestIndicesBuffer);
        clReleaseMemObject(distancesBuffer);
        clReleaseMemObject(buffer2);

        std::sort(l_matches->begin(), l_matches->end(), [](Match &a, Match &b) {
            return a.distance < b.distance;
        });

        uint32_t* counter = new uint32_t[Core::CHUNKS_COUNT] { 0 };
        fi = 0;
        for (size_t m = 0; m < l_matches->size; m++) {
            const KeyPoint &kp = keyPoints[(*l_matches)[m].index2];
            uint32_t x = std::min((uint32_t)kp.x / chunkWidth, Core::CHUNKS_PER_SIDE - 1);
            uint32_t y = std::min((uint32_t)kp.y / chunkHeight, Core::CHUNKS_PER_SIDE - 1);

            uint32_t index = y * Core::CHUNKS_PER_SIDE + x;
            double distance = (*l_matches)[m].distance;
            if (counter[index] <= Core::MATCHES_PER_CHUNK && distance < 30.0) {
                double t = (distance > 5.0) ? ((distance - 5.0) * 0.04) : 0.0;
                double threshold = Core::MATCHES_PER_CHUNK * (1.0 - 0.5 * t);
                if (counter[index] <= threshold) {
                    (*l_matches)[fi] = (*l_matches)[m];
                    counter[index]++;
                    fi++;
                }
            }
        }

        delete [] counter;
        l_matches->size = fi;
        l_matches->shrink();
        matchesIndex++;
    }

    return matches;
}