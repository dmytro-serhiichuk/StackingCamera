//
// Created by sedv2 on 13.01.2026.
//

#ifndef STACKINGCAMERA_MATCHING_H
#define STACKINGCAMERA_MATCHING_H

#include "collections/list.h"
#include "m-brisk/m-brisk.h"
#include "core.h"

namespace Matching {
    typedef struct Match {
        uint32_t index1;
        uint32_t index2; // key descriptor
        uint32_t distance;

        Match();
        Match(uint32_t _i1, uint32_t _i2, uint32_t _dist);
    } Match;

    typedef struct MatchingCLBuffers {
        cl_mem closestIndicesBuffer;
        cl_mem distancesBuffer;
        int32_t* closestIndices;
        int32_t* distances;

        size_t size;
        size_t count;

        MatchingCLBuffers(size_t c);
        ~MatchingCLBuffers();
    } MatchingCLBuffers;

    Buffer<Buffer<Match>>* match(List<Core::Data> &sources, uint32_t bestIndex);
}

#endif //STACKINGCAMERA_MATCHING_H
