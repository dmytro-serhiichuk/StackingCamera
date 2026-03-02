//
// Created by sedv2 on 02.03.2026.
//

#include "matches-validation.h"

namespace Validation {
    namespace {
        const double WARN_INLIERS_PERCENTAGE = 0.15;
        const double BAD_INLIERS_PERCENTAGE  = 0.05;

        const int WARN_INLIERS_NUMBER = 40;
        const int BAD_INLIERS_NUMBER  = 15;

        const int GRID = 10;

        const double WARN_INLIERS_CV = 1.5;
        const double BAD_INLIERS_CV  = 3.0;

        double getCV(Buffer<Matching::Match> &matches, RANSAC::InliersInfo &inliersInfo, Core::Data &data) {
            int *counts = new int[GRID * GRID]();

            auto& bmp = *data.bitmapPtr;

            double gridWidth  = (double)bmp.width  / (double)GRID;
            double gridHeight = (double)bmp.height / (double)GRID;

            for (size_t i = 0; i < matches.size; i++) {
                if (!inliersInfo.mask[i]) continue;
                auto &kp = (*data.keyPoints)[matches[i].index1];
                int xIdx = std::min((int)(kp.x / gridWidth), GRID - 1);
                int yIdx = std::min((int)(kp.y / gridHeight), GRID - 1);
                counts[yIdx * GRID + xIdx]++;
            }

            double sum = 0.0;
            int totalCells = GRID * GRID;
            for (size_t i = 0; i < totalCells; i++) {
                sum += counts[i];
            }

            double mean = sum / totalCells;

            double sqSum = 0.0;
            for (size_t i = 0; i < totalCells; i++) {
                double diff = counts[i] - mean;
                sqSum += diff * diff;
            }

            delete [] counts;

            double sigma = std::sqrt(sqSum / totalCells);
            return (mean > 0.0 ? sigma / mean : 1000.0);
        }

        MatchesValidationInfo validate(Buffer<Matching::Match> &matches, RANSAC::InliersInfo &inliersInfo, Core::Data &data) {
            MatchesValidationInfo info {};

            if (inliersInfo.count <= BAD_INLIERS_NUMBER) {
                info.inliersNumber = Status::BAD;
            } else if (inliersInfo.count <= WARN_INLIERS_NUMBER) {
                info.inliersNumber = Status::WARNING;
            } else {
                info.inliersNumber = Status::OK;
            }

            double inliersPercentage = (double )inliersInfo.count / (double)matches.size;
            if (inliersPercentage <= BAD_INLIERS_PERCENTAGE) {
                info.inliersPercentage = Status::BAD;
            } else if (inliersPercentage <= WARN_INLIERS_PERCENTAGE) {
                info.inliersPercentage = Status::WARNING;
            } else {
                info.inliersPercentage = Status::OK;
            }

            // TODO: improve method for distribution calculation
            double cv = getCV(matches, inliersInfo, data);
            if (cv >= BAD_INLIERS_CV) {
                info.evenDistribution = Status::BAD;
            } else if (cv >= WARN_INLIERS_CV) {
                info.evenDistribution = Status::WARNING;
            } else {
                info.evenDistribution = Status::OK;
            }

            return info;
        }
    }

    std::vector<MatchesValidationInfo>
    validateMatches(List<Core::Data> &sources, Buffer<Buffer<Matching::Match>> &matches,
                    Buffer<RANSAC::Result> &ransacResults, int32_t referenceIndex)
    {
        std::vector<MatchesValidationInfo> infos {};
        infos.reserve(matches.size);

        int32_t matchesIndex = 0;
        for (int32_t i = 0; i < sources.size; i++) {
            if (i == referenceIndex) continue;
            infos.push_back(validate(
                    matches[matchesIndex],
                    ransacResults[matchesIndex].info,
                    *sources.buffer[i]
            ));
            matchesIndex++;
        }

        return infos;
    }
}

