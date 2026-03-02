//
// Created by sedv2 on 13.01.2026.
//

#include "ransac.h"
#include <vector>
#include <random>
#include "core.h"

namespace RANSAC {
    namespace {
        void shuffleMatches(Buffer<Matching::Match> &matches, int32_t* indices) {
            auto seed = std::chrono::system_clock::now().time_since_epoch().count();
            std::default_random_engine generator(seed);

            std::shuffle(indices, indices + matches.size, generator);
        }

        Eigen::Matrix3d findHomography(Buffer<Matching::Match> &matches, Buffer<KeyPoint> &kps1, Buffer<KeyPoint> &kps2, int32_t* indices) {
            Eigen::MatrixXd A(8, 9);

            for (size_t i = 0; i < 4; i++) {
                KeyPoint &p1 = kps1[matches[indices[i]].index1];
                KeyPoint &p2 = kps2[matches[indices[i]].index2];

                A.row(2 * i) << p1.x, p1.y, 1.0, 0.0, 0.0, 0.0, -p2.x * p1.x, -p2.x * p1.y, -p2.x;
                A.row(2 * i + 1) << 0.0, 0.0, 0.0, p1.x, p1.y, 1.0, -p2.y * p1.x, -p2.y * p1.y, -p2.y;
            }

            Eigen::JacobiSVD<Eigen::MatrixXd> svd(A, Eigen::ComputeFullV);

            Eigen::VectorXd h = svd.matrixV().col(8);

            h /= h[8];

            Eigen::Matrix3d H;
            H << h[0], h[1], h[2],
                    h[3], h[4], h[5],
                    h[6], h[7], h[8];
            return H;
        }

        Eigen::Vector2d projectPoint(Eigen::Matrix3d &H, KeyPoint &point) {
            Eigen::Vector3d pointExt(point.x, point.y, 1.0);
            Eigen::Vector3d projected = H * pointExt;
            return Eigen::Vector2d(projected[0] / projected[2], projected[1] / projected[2]);
        }

        InliersInfo findInliers(
                Buffer<Matching::Match> &matches,
                Buffer<KeyPoint> &kps1,
                Buffer<KeyPoint> &kps2,
                Eigen::Matrix3d &H
        ) {
            InliersInfo info;
            info.mask.resize(matches.size, false);
            info.count = 0;
            info.errorFactor = 0.0;

            for (size_t i = 0; i < matches.size; i++) {
                Eigen::Vector2d projectedVector = projectPoint(H, kps1[matches[i].index1]);
                Eigen::Vector2d point2Vector(kps2[matches[i].index2].x, kps2[matches[i].index2].y);

                double distance = (projectedVector - point2Vector).norm();
                double fDistance = distance * distance * distance;

                if (distance <= Core::RANSAC_THRESHOLD) {
                    info.count++;
                    info.mask[i] = true;
                    info.errorFactor += fDistance;
                }
            }

            return info;
        }

        Eigen::Matrix3d refineHomographyWithInliers(
                Buffer<Matching::Match> &matches,
                Buffer<KeyPoint> &kps1,
                Buffer<KeyPoint> &kps2,
                InliersInfo &inliersMask
        ) {
            Eigen::MatrixXd A(inliersMask.count * 2, 9);
            int rowIdx = 0;

            for (size_t i = 0; i < matches.size; i++) {
                if (!inliersMask.mask[i]) continue;

                KeyPoint &p1 = kps1[matches[i].index1];
                KeyPoint &p2 = kps2[matches[i].index2];

                A.row(rowIdx) << p1.x, p1.y, 1.0, 0.0, 0.0, 0.0, -p2.x * p1.x, -p2.x * p1.y, -p2.x;
                A.row(rowIdx + 1) << 0.0, 0.0, 0.0, p1.x, p1.y, 1.0, -p2.y * p1.x, -p2.y * p1.y, -p2.y;

                rowIdx += 2;
            }

            Eigen::JacobiSVD<Eigen::MatrixXd> svd(A, Eigen::ComputeFullV);
            Eigen::VectorXd h = svd.matrixV().col(8);

            h /= h[8];

            Eigen::Matrix3d H;
            H << h[0], h[1], h[2],
                    h[3], h[4], h[5],
                    h[6], h[7], h[8];

            return H;
        }

        Result computeHomography(Buffer<Matching::Match> &matches, Buffer<KeyPoint> &kps1,
                                          Buffer<KeyPoint> &kps2) {
            Eigen::Matrix3d bestH;
            InliersInfo bestInliers;

            int32_t* indices = new int32_t[matches.size];
            for (size_t i = 0; i < matches.size; i++) {
                indices[i] = i;
            }

            for (size_t i = 0; i < Core::RANSAC_ITERATIONS; i++) {
                shuffleMatches(matches, indices);
                Eigen::Matrix3d H = findHomography(matches, kps1, kps2, indices);
                InliersInfo inlierInfo = findInliers(matches, kps1, kps2, H);

                if ((inlierInfo.count > bestInliers.count) || (inlierInfo.count == bestInliers.count && inlierInfo.errorFactor < bestInliers.errorFactor)) {
                    bestH = H;
                    bestInliers = inlierInfo;
                }
            }

            Eigen::Matrix3d refinedH = refineHomographyWithInliers(matches, kps1, kps2, bestInliers);

            InliersInfo refinedInfo = findInliers(matches, kps1, kps2, refinedH);

            if (refinedInfo.count >= bestInliers.count && refinedInfo.errorFactor <= bestInliers.errorFactor) {
                bestH = refinedH;
                bestInliers = refinedInfo;
            }

            JNIHelper::getInstance()->writeMessageToLog(false, "Number of inliers: %zd\nError: %f\n", bestInliers.count, bestInliers.errorFactor);

            delete [] indices;

            return { bestInliers, bestH };
        }
    }

    Buffer<Result> *
    computeHomographyMatrices(List<Core::Data> &sources, uint32_t bestIndex, Buffer<Buffer<Matching::Match>> &matches) {
        auto results = new Buffer<Result>(matches.size);
        results->size = matches.size;

        size_t matchesIndex = 0;
        for (size_t i = 0; i < sources.size; i++) {
            if (i == bestIndex) continue;
            JNIHelper::getInstance()->writeMessageToLog(false, "Starting computing homography for image %zd", i);

            results->buffer[matchesIndex] = computeHomography(
                matches[matchesIndex],
                *sources.buffer[i]->keyPoints,
                *sources.buffer[bestIndex]->keyPoints
            );
            matchesIndex++;
        }

        return results;
    }
}

