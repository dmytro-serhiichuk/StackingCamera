//
// Created by sedv2 on 28.02.2026.
//

#include "homography-validation.h"
#include <vector>

namespace {
    const double MIN_WARN_AREA_RATION = 0.8;
    const double MAX_WARN_AREA_RATION = 1.3;

    const double MIN_BAD_AREA_RATION  = 0.4;
    const double MAX_BAD_AREA_RATION  = 3.0;

    const double MAX_WARN_PERSPECTIVE_ERROR = 0.002;
    const double MAX_BAD_PERSPECTIVE_ERROR  = 0.005;

    const double MAX_WARN_ANISOTROPY_DEFORMATION = 1.3;
    const double MAX_BAD_ANISOTROPY_DEFORMATION  = 2.0;

    const double MAX_WARN_TRANSLATION_RATIO = 0.3;
    const double MAX_BAD_TRANSLATION_RATIO  = 0.6;

    const double MAX_WARN_SHEAR_ANGLE = 5.0;
    const double MAX_BAD_SHEAR_ANGLE  = 15.0;

    struct HomographyMetrics {
        double rotation_deg;
        double scale_x, scale_y;
        double translation_x, translation_y;
        double perspective_strength;
        double svd_ratio;
        double determinant;
        double shear_angle;
    };

    HomographyMetrics analyzeHomographyMatrix(const Eigen::Matrix3d& H) {
        HomographyMetrics m {};

        m.determinant = H.determinant();

        m.scale_x = std::sqrt(H(0,0)*H(0,0) + H(1,0)*H(1,0));
        m.scale_y = std::sqrt(H(0,1)*H(0,1) + H(1,1)*H(1,1));

        m.rotation_deg = std::atan2(H(1,0), H(0,0)) * 180.0 / M_PI;

        m.translation_x = H(0,2);
        m.translation_y = H(1,2);

        m.perspective_strength = std::sqrt(H(2,0)*H(2,0) + H(2,1)*H(2,1));

        Eigen::Matrix2d affine = H.block<2,2>(0,0);
        Eigen::JacobiSVD<Eigen::Matrix2d> svd(affine);
        auto singular = svd.singularValues();
        m.svd_ratio = (singular(1) > 1e-10) ? singular(0) / singular(1) : 999.0;

        Eigen::Vector2d col0 = affine.col(0);
        Eigen::Vector2d col1 = affine.col(1);

        double sx = col0.norm();
        Eigen::Vector2d q0 = col0 / sx;

        double shear = q0.dot(col1);
        double sy = (col1 - shear * q0).norm();

        m.shear_angle = std::atan2(shear, sy) * 180.0 / M_PI;

        return m;
    }

    struct CornersAnalysis {
        double max_displacement;
        double avg_displacement;
        double area_ratio;
        bool convex;
    };

    Eigen::Vector2d applyHomography(const Eigen::Matrix3d& H, const Eigen::Vector2d& pt) {
        Eigen::Vector3d p(pt.x(), pt.y(), 1.0);
        Eigen::Vector3d result = H * p;
        return { result.x() / result.z(), result.y() / result.z() };
    }

    double polygonArea(const std::vector<Eigen::Vector2d>& pts) {
        double area = 0;
        int n = pts.size();
        for (int i = 0; i < n; i++) {
            int j = (i + 1) % n;
            area += pts[i].x() * pts[j].y();
            area -= pts[j].x() * pts[i].y();
        }
        return std::abs(area) / 2.0;
    }

    bool isConvex(const std::vector<Eigen::Vector2d>& pts) {
        int n = pts.size();
        int sign = 0;
        for (int i = 0; i < n; i++) {
            Eigen::Vector2d d1 = pts[(i+1)%n] - pts[i];
            Eigen::Vector2d d2 = pts[(i+2)%n] - pts[(i+1)%n];
            double cross = d1.x()*d2.y() - d1.y()*d2.x();
            int s = (cross > 0) ? 1 : -1;
            if (sign == 0) sign = s;
            else if (sign != s) return false;
        }
        return true;
    }

    CornersAnalysis analyzeCorners(const Eigen::Matrix3d& H, double width, double height) {
        std::vector<Eigen::Vector2d> original = {
                {0, 0}, {width, 0}, {width, height}, {0, height}
        };

        std::vector<Eigen::Vector2d> transformed;
        for (const auto& pt : original) {
            transformed.push_back(applyHomography(H, pt));
        }

        CornersAnalysis result {};

        double max_d = 0, sum_d = 0;
        for (int i = 0; i < 4; i++) {
            double d = (transformed[i] - original[i]).norm();
            max_d = std::max(max_d, d);
            sum_d += d;
        }
        result.max_displacement = max_d;
        result.avg_displacement = sum_d / 4.0;

        double orig_area  = polygonArea(original);
        double trans_area = polygonArea(transformed);
        result.area_ratio = trans_area / orig_area;

        result.convex = isConvex(transformed);

        return result;
    }
}

namespace HomographyValidation {
    ValidationInfo validate(Eigen::Matrix3d &matrix, int32_t width, int32_t height) {
        ValidationInfo info {};

        auto mat = analyzeHomographyMatrix(matrix);
        auto geo = analyzeCorners(matrix, width, height);

        info.isConvex = geo.convex;
        info.mirrored = mat.determinant < 0;

        auto translation_x_ratio = std::abs(mat.translation_x) / width;
        auto translation_y_ratio = std::abs(mat.translation_y) / height;

        if (translation_x_ratio >= MAX_BAD_TRANSLATION_RATIO) {
            info.translationX = Status::BAD;
        } else if (translation_x_ratio >= MAX_WARN_TRANSLATION_RATIO) {
            info.translationX = Status::WARNING;
        } else {
            info.translationX = Status::OK;
        }

        if (translation_y_ratio >= MAX_BAD_TRANSLATION_RATIO) {
            info.translationY = Status::BAD;
        } else if (translation_y_ratio >= MAX_WARN_TRANSLATION_RATIO) {
            info.translationY = Status::WARNING;
        } else {
            info.translationY = Status::OK;
        }

        if (mat.perspective_strength >= MAX_BAD_PERSPECTIVE_ERROR) {
            info.perspective = Status::BAD;
        } else if (mat.perspective_strength >= MAX_WARN_PERSPECTIVE_ERROR) {
            info.perspective = Status::WARNING;
        } else {
            info.perspective = Status::OK;
        }

        if (geo.area_ratio <= MIN_BAD_AREA_RATION || geo.area_ratio >= MAX_BAD_AREA_RATION) {
            info.scale = Status::BAD;
        } else if (geo.area_ratio <= MIN_WARN_AREA_RATION || geo.area_ratio >= MAX_WARN_AREA_RATION) {
            info.scale = Status::WARNING;
        } else {
            info.scale = Status::OK;
        }

        if (mat.shear_angle >= MAX_BAD_SHEAR_ANGLE) {
            info.shear = Status::BAD;
        } else if (mat.shear_angle >= MAX_WARN_SHEAR_ANGLE) {
            info.shear = Status::WARNING;
        } else {
            info.shear = Status::OK;
        }

        if (mat.svd_ratio >= MAX_BAD_ANISOTROPY_DEFORMATION) {
            info.anisotropy = Status::BAD;
        } else if (mat.svd_ratio >= MAX_WARN_ANISOTROPY_DEFORMATION) {
            info.anisotropy = Status::WARNING;
        } else {
            info.anisotropy = Status::OK;
        }

        return info;
    }
}

