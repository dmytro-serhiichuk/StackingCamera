package com.sedv.stackingcamera.stacking

class BitmapInfo(
    val width: Int,
    val height: Int,
    var name: String = "null",
    var score: Int = -1,
    var isReferenceFrame: Boolean = false,
    var bitmapValidationInfo: BitmapValidationInfo? = null
) {
    val isAnalyzed: Boolean get() = score > 0
}

class BitmapValidationInfo(
    val scale: BitmapValidationStatus,
    val translationX: BitmapValidationStatus,
    val translationY: BitmapValidationStatus,
    val perspective: BitmapValidationStatus,
    val shear: BitmapValidationStatus,
    val anisotropy: BitmapValidationStatus,
    val isConvex: Boolean,
    val mirrored: Boolean,
    val inliersPercentage: BitmapValidationStatus,
    val inliersNumber: BitmapValidationStatus,
    val evenDistribution: BitmapValidationStatus,
    val props: List<BitmapValidationStatus> = listOf(
        scale, translationX, translationY, perspective, shear,
        anisotropy, inliersPercentage, inliersNumber, evenDistribution
    )
) {
    fun getMessage(): String {
        if (!isConvex || mirrored) {
            return "Image can not be aligned with the reference image"
        }

        var message = ""
        if (perspective == BitmapValidationStatus.BAD) {
            message += "Perspective distortion is too strong for reliable alignment\n"
        } else if (perspective == BitmapValidationStatus.WARNING) {
            message += "Strong perspective differences may cause misalignment of objects at different depths\n"
        }

        if (anisotropy == BitmapValidationStatus.BAD) {
            message += "Non-uniform image scaling is too strong\n"
        } else if (anisotropy == BitmapValidationStatus.WARNING) {
            message += "Uneven scaling may reduce alignment accuracy\n"
        }

        if (shear == BitmapValidationStatus.BAD) {
            message += "Image is too strongly skewed\n"
        } else if (shear == BitmapValidationStatus.WARNING) {
            message += "Image skew may slightly reduce output quality\n"
        }

        if (scale == BitmapValidationStatus.BAD) {
            message += "Image scale difference is too large\n"
        } else if (scale == BitmapValidationStatus.WARNING) {
            message += "Significant scaling may reduce image detail\n"
        }

        if (translationX == BitmapValidationStatus.BAD || translationY == BitmapValidationStatus.BAD) {
            message += "Image is shifted too far from the reference frame\n"
        } else {
            if (translationX == BitmapValidationStatus.WARNING) {
                message += "Horizontal shift may affect alignment quality\n"
            }
            if (translationY == BitmapValidationStatus.WARNING) {
                message += "Vertical shift may affect alignment quality\n"
            }
        }

        if (inliersPercentage == BitmapValidationStatus.BAD) {
            message += "Too few reliable matches were found\n"
        } else if (inliersPercentage == BitmapValidationStatus.WARNING) {
            message += "A low proportion of reliable matches may indicate poor overall matching quality\n"
        }

        if (inliersNumber == BitmapValidationStatus.BAD) {
            message += "Too few matches were found between the images\n"
        } else if (inliersNumber == BitmapValidationStatus.WARNING) {
            message += "A small number of matches may make precise alignment impossible\n"
        }

        if (evenDistribution == BitmapValidationStatus.BAD) {
            message += "Matches cover only a small part of the image\n"
        } else if (evenDistribution == BitmapValidationStatus.WARNING) {
            message += "Matches are unevenly distributed, which may cause poor alignment in some areas of the image\n"
        }

        return message.dropLast(1)
    }

    companion object {
        const val FIELDS_PER_MATCH_RESULT = 12
    }
}

enum class BitmapValidationStatus(val value: Int) {
    OK(0),
    WARNING(1),
    BAD(2);

    companion object {
        fun fromInt(value: Int) = BitmapValidationStatus.entries.first { it.value == value }
    }
}