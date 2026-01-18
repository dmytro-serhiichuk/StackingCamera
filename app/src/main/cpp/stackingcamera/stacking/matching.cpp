//
// Created by sedv2 on 13.01.2026.
//

#include "matching.h"
#include "core.h"
#include <algorithm>

namespace Matching {
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

    MatchingCLBuffers::MatchingCLBuffers(size_t c) {
        count = c;
        size = count * sizeof(int32_t);

        closestIndices = new int32_t[count]();
        distances      = new int32_t[count]();

        closestIndicesBuffer = CL::createBuffer(
                CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                size, nullptr
        );
        distancesBuffer = CL::createBuffer(
                CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
                size, nullptr
        );
    }

    MatchingCLBuffers::~MatchingCLBuffers() {
        delete [] closestIndices;
        delete [] distances;

        clReleaseMemObject(closestIndicesBuffer);
        clReleaseMemObject(distancesBuffer);
    }


    Buffer<Buffer<Match>> *match(List<Core::Data> &sources, uint32_t bestIndex) {
        auto &bmp = *sources.buffer[bestIndex]->bitmapPtr;
        auto &bestKeypoints = *sources.buffer[bestIndex]->keyPoints;

        size_t matchesIndex = 0;
        auto matches = new Buffer<Buffer<Match>>(sources.size - 1);
        matches->size = sources.size - 1;

        Descriptors& descriptors1 = *sources.buffer[bestIndex]->descriptors;

        cl_mem buffer1 = CL::createBuffer(
                CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                descriptors1.sizeOf(), descriptors1.buffer
        );

        cl_kernel kernel = CL::createKernel("find_closest_descriptors");

        clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer1);
        clSetKernelArg(kernel, 4, sizeof(int32_t), &descriptors1.count);

        uint32_t chunkWidth = bmp.width / Core::CHUNKS_PER_SIDE;
        uint32_t chunkHeight = bmp.height / Core::CHUNKS_PER_SIDE;

        MatchingCLBuffers matchingClBuffers {descriptors1.count};

        for (size_t i = 0; i < sources.size; i++) {
            if (i == bestIndex) continue;

            Descriptors& descriptors2 = *sources.buffer[i]->descriptors;

            cl_mem buffer2 = CL::createBuffer(
                    CL_MEM_HOST_NO_ACCESS | CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                    descriptors2.sizeOf(), descriptors2.buffer
            );

            clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer2);
            clSetKernelArg(kernel, 2, sizeof(cl_mem), &matchingClBuffers.closestIndicesBuffer);
            clSetKernelArg(kernel, 3, sizeof(cl_mem), &matchingClBuffers.distancesBuffer);
            clSetKernelArg(kernel, 5, sizeof(int32_t), &descriptors2.count);

            cl_event matchingFinished, indicesRead, distancesRead;

            clEnqueueNDRangeKernel(
                    CL::queue, kernel, 1, nullptr,
                    &descriptors1.count, nullptr,
                    0, nullptr, &matchingFinished
            );
            clEnqueueReadBuffer(
                CL::queue, matchingClBuffers.closestIndicesBuffer,
                CL_FALSE, 0, matchingClBuffers.size, matchingClBuffers.closestIndices,
                1, &matchingFinished,&indicesRead
            );
            clEnqueueReadBuffer(
                    CL::queue, matchingClBuffers.distancesBuffer,
                    CL_FALSE, 0, matchingClBuffers.size, matchingClBuffers.distances,
                    1, &matchingFinished,&distancesRead
            );

            cl_event wait_list[2] { distancesRead, indicesRead };
            clWaitForEvents(2, wait_list);

            clReleaseEvent(matchingFinished);
            clReleaseEvent(indicesRead);
            clReleaseEvent(distancesRead);
            clReleaseMemObject(buffer2);

            Buffer<Match>& currentMatches = (*matches)[matchesIndex];
            currentMatches.buffer = new Match[matchingClBuffers.count];
            currentMatches.size = matchingClBuffers.count;
            currentMatches.capacity = matchingClBuffers.count;

            size_t fi = 0;
            for (uint32_t j = 0; j < descriptors1.count; j++) {
                if (matchingClBuffers.distances[j] < 30) {
                    currentMatches[fi] = Match(matchingClBuffers.closestIndices[j], j, matchingClBuffers.distances[j]);
                    fi++;
                }
            }
            currentMatches.size = fi;

            std::sort(currentMatches.begin(), currentMatches.end(), [](Match &a, Match &b) {
                return a.distance < b.distance;
            });

            auto counter = new uint32_t[Core::CHUNKS_COUNT]();
            fi = 0;
            for (size_t m = 0; m < currentMatches.size; m++) {
                const KeyPoint &kp = bestKeypoints[currentMatches[m].index2];
                uint32_t x = std::min((uint32_t)kp.x / chunkWidth, Core::CHUNKS_PER_SIDE - 1);
                uint32_t y = std::min((uint32_t)kp.y / chunkHeight, Core::CHUNKS_PER_SIDE - 1);

                uint32_t index = y * Core::CHUNKS_PER_SIDE + x;
                double distance = currentMatches[m].distance;
                if (counter[index] <= Core::MATCHES_PER_CHUNK) {
                    double t = (distance > 5.0) ? ((distance - 5.0) * 0.04) : 0.0;
                    double threshold = Core::MATCHES_PER_CHUNK * (1.0 - 0.5 * t);
                    if (counter[index] <= threshold) {
                        currentMatches[fi] = currentMatches[m];
                        counter[index]++;
                        fi++;
                    }
                }
            }

            delete [] counter;
            currentMatches.size = fi;
            currentMatches.shrink();
            matchesIndex++;
        }

        clReleaseKernel(kernel);
        clReleaseMemObject(buffer1);

        return matches;
    }
}

