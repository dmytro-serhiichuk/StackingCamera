//
// Created by sedv2 on 13.01.2026.
//

#include "ransac.h"
#include <vector>
#include <random>
#include "core.h"

namespace RANSAC {
    namespace {
        typedef Eigen::Matrix<double, 8, 9> Matrix8x9;
        typedef Eigen::Matrix<double, Eigen::Dynamic, 9> MatrixInliers;
        typedef Eigen::Vector<double, 9> Vector9;

        auto seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::default_random_engine gen(static_cast<unsigned long>(seed));

        inline void shuffleMatches(size_t size, int32_t* indices) {
            std::shuffle(indices, indices + size, gen);
        }

        inline Eigen::Matrix3d findHomography(
                const Buffer<Matching::Match> &matches, const Buffer<KeyPoint> &kps1,
                const Buffer<KeyPoint> &kps2, const int32_t* indices
        ) {
            Matrix8x9 A = Matrix8x9();

            for (Eigen::Index i = 0; i < 4; i++) {
                const auto &match = matches[indices[i]];
                const auto &p1 = kps1[match.index1];
                const auto &p2 = kps2[match.index2];

                A.row(2 * i) << p1.x, p1.y, 1.0, 0.0, 0.0, 0.0, -p2.x * p1.x, -p2.x * p1.y, -p2.x;
                A.row(2 * i + 1) << 0.0, 0.0, 0.0, p1.x, p1.y, 1.0, -p2.y * p1.x, -p2.y * p1.y, -p2.y;
            }

            Eigen::JacobiSVD<Matrix8x9> svd(A, Eigen::ComputeFullV);
            Vector9 h = svd.matrixV().col(8);
            h /= h(8);
            Eigen::Matrix3d H;
            H << h(0), h(1), h(2),
                 h(3), h(4), h(5),
                 h(6), h(7), h(8);
            return H;
        }

        inline void findInliers(
                const Buffer<Matching::Match> &matches,
                const Buffer<KeyPoint> &kps1,
                const Buffer<KeyPoint> &kps2,
                const Eigen::Matrix3d &H,
                InliersInfo &info,
                const int lastBestCount
        ) {
            info.count = 0;
            info.errorFactor = 0.0;

            const double h00 = H(0,0), h01 = H(0,1), h02 = H(0,2);
            const double h10 = H(1,0), h11 = H(1,1), h12 = H(1,2);
            const double h20 = H(2,0), h21 = H(2,1), h22 = H(2,2);

            for (size_t i = 0; i < matches.size; i++) {
                const auto &match = matches[i];
                const auto &kp1 = kps1[match.index1];
                const auto &kp2 = kps2[match.index2];

                double w = h20 * kp1.x + h21 * kp1.y + h22;
                    if (std::abs(w) < 1e-10) {
                    info.mask[i] = false;
                    continue;
                }

                double invW = 1.0 / w;
                double dx = (h00 * kp1.x + h01 * kp1.y + h02) * invW - kp2.x;
                double dy = (h10 * kp1.x + h11 * kp1.y + h12) * invW - kp2.y;
                double distSq = dx*dx + dy*dy;

                if (distSq <= Settings::RANSAC_THRESHOLD * Settings::RANSAC_THRESHOLD) {
                    info.count++;
                    info.errorFactor += distSq;
                    info.mask[i] = true;
                } else {
                    info.mask[i] = false;
                }

                if (info.count + (matches.size - i - 1) < lastBestCount) {
                    break;
                }
            }
        }

        inline Eigen::Matrix3d refineHomographyWithInliers(
                const Buffer<Matching::Match> &matches,
                const Buffer<KeyPoint> &kps1,
                const Buffer<KeyPoint> &kps2,
                const InliersInfo &inliersMask
        ) {
            MatrixInliers A(inliersMask.count * 2, 9);
            int rowIdx = 0;

            for (size_t i = 0; i < matches.size; i++) {
                if (!inliersMask.mask[i]) continue;

                const auto &match = matches[i];
                const auto &p1 = kps1[match.index1];
                const auto &p2 = kps2[match.index2];

                A.row(rowIdx) << p1.x, p1.y, 1.0, 0.0, 0.0, 0.0, -p2.x * p1.x, -p2.x * p1.y, -p2.x;
                A.row(rowIdx + 1) << 0.0, 0.0, 0.0, p1.x, p1.y, 1.0, -p2.y * p1.x, -p2.y * p1.y, -p2.y;

                rowIdx += 2;
            }

            Eigen::JacobiSVD<Eigen::MatrixXd> svd(A, Eigen::ComputeFullV);
            Eigen::VectorXd h = svd.matrixV().col(8);

            h /= h[8];

            Eigen::Matrix3d H;
            H << h(0), h(1), h(2),
                 h(3), h(4), h(5),
                 h(6), h(7), h(8);

            return H;
        }

        Result computeHomography(Buffer<Matching::Match> &matches, Buffer<KeyPoint> &kps1,
                                          Buffer<KeyPoint> &kps2) {
            InliersInfo bestInfo {0, 100000.0, matches.size};
            Eigen::Matrix3d bestH = Eigen::Matrix3d::Identity();

            auto indices = new int32_t[matches.size];
            for (int32_t i = 0; i < matches.size; i++) indices[i] = i;

            InliersInfo info {0, 100000.0, matches.size};

            for (size_t i = 0; i < Settings::RANSAC_ITERATIONS; i++) {
                shuffleMatches(matches.size, indices);

                Eigen::Matrix3d H = findHomography(matches, kps1, kps2, indices);
                findInliers(matches, kps1, kps2, H, info, bestInfo.count);

                if ((info.count > bestInfo.count) ||
                    (info.count == bestInfo.count &&
                     info.errorFactor < bestInfo.errorFactor)) {
                    bestH = H;
                    bestInfo.count = info.count;
                    bestInfo.errorFactor = info.errorFactor;
                    auto temp = bestInfo.mask;
                    bestInfo.mask = info.mask;
                    info.mask = temp;
                }
            }

            auto H = refineHomographyWithInliers(matches, kps1, kps2, bestInfo);

            findInliers(matches, kps1, kps2, H, info, bestInfo.count);

            if (info.count > bestInfo.count ||
               (info.count == bestInfo.count && info.errorFactor < bestInfo.errorFactor)) {
                bestH = H;
                bestInfo.count = info.count;
                bestInfo.errorFactor = info.errorFactor;
                auto temp = bestInfo.mask;
                bestInfo.mask = info.mask;
                info.mask = temp;
            }

            Core::getLogger()->log(false, "Number of inliers: %zd\nError: %f\n", bestInfo.count, bestInfo.errorFactor);

            delete [] indices;

            return { bestInfo, bestH };
        }
    }

    Buffer<Result> *
    computeHomographyMatrices(List<Core::Data> &sources, uint32_t bestIndex, Buffer<Buffer<Matching::Match>> &matches) {
        auto results = new Buffer<Result>(matches.size);
        results->size = matches.size;

        size_t matchesIndex = 0;
        for (size_t i = 0; i < sources.size; i++) {
            if (i == bestIndex) continue;
            Core::getLogger()->log(false, "Starting computing homography for image %zd", i);

            results->buffer[matchesIndex] = computeHomography(
                matches[matchesIndex],
                *sources.buffer[i]->keyPoints,
                *sources.buffer[bestIndex]->keyPoints
            );
            matchesIndex++;
        }

        return results;
    }

    InliersInfo::InliersInfo(int ic, double e, size_t ms) {
        count = ic;
        errorFactor = e;
        maskSize = ms;
        mask = new bool[maskSize];
    }
    InliersInfo::InliersInfo(const InliersInfo &other) {
        count = other.count;
        errorFactor = other.errorFactor;
        maskSize = other.maskSize;
        mask = new bool[maskSize];
        memcpy(mask, other.mask, maskSize);
    }
    InliersInfo::InliersInfo(InliersInfo &&other) noexcept {
        count = other.count;
        errorFactor = other.errorFactor;
        maskSize = other.maskSize;
        mask = other.mask;
        other.mask = nullptr;
    }
    InliersInfo::~InliersInfo() {
        count = 0;
        errorFactor = 0.0;
        maskSize = 0;
        delete []mask;
        mask = nullptr;
    }
    InliersInfo &InliersInfo::operator=(InliersInfo &&other) noexcept {
        if (this != &other) {
            count = other.count;
            errorFactor = other.errorFactor;
            maskSize = other.maskSize;
            mask = other.mask;
            other.mask = nullptr;
        }
        return *this;
    }
    InliersInfo &InliersInfo::operator=(const InliersInfo &other) {
        if (this != &other) {
            count = other.count;
            errorFactor = other.errorFactor;
            maskSize = other.maskSize;
            mask = new bool[maskSize];
            memcpy(mask, other.mask, maskSize);
        }
        return *this;
    }

}

