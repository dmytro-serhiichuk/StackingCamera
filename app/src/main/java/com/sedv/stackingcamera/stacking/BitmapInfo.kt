package com.sedv.stackingcamera.stacking

class BitmapInfo(
    val width: Int,
    val height: Int,
    var name: String = "null",
    var score: Int = -1,
    var isReferenceFrame: Boolean = false,
    var homographyValidationInfo: HomographyValidationInfo? = null
) {
    val isAnalyzed: Boolean get() = score > 0
}

class HomographyValidationInfo(
    val scale: HomographyValidationStatus,
    val translationX: HomographyValidationStatus,
    val translationY: HomographyValidationStatus,
    val perspective: HomographyValidationStatus,
    val shear: HomographyValidationStatus,
    val anisotropy: HomographyValidationStatus,
    val isConvex: Boolean,
    val mirrored: Boolean,
    val props: List<HomographyValidationStatus> = listOf(
        scale, translationX, translationY, perspective, shear, anisotropy
    )
) {
    fun getMessage(): String {
        if (!isConvex || mirrored) {
            return "Image can not be aligned with the reference image"
        }

        var message = ""
        if (perspective == HomographyValidationStatus.BAD) {
            message += "Perspective distortion is too strong for reliable alignment\n"
        } else if (perspective == HomographyValidationStatus.WARNING) {
            message += "Strong perspective differences may cause misalignment of objects at different depths\n"
        }

        if (anisotropy == HomographyValidationStatus.BAD) {
            message += "Non-uniform image scaling is too strong\n"
        } else if (anisotropy == HomographyValidationStatus.WARNING) {
            message += "Uneven scaling may reduce alignment accuracy\n"
        }

        if (shear == HomographyValidationStatus.BAD) {
            message += "Image is too strongly skewed\n"
        } else if (shear == HomographyValidationStatus.WARNING) {
            message += "Image skew may slightly reduce output quality\n"
        }

        if (scale == HomographyValidationStatus.BAD) {
            message += "Image scale difference is too large\n"
        } else if (scale == HomographyValidationStatus.WARNING) {
            message += "Significant scaling may reduce image detail\n"
        }

        if (translationX == HomographyValidationStatus.BAD || translationY == HomographyValidationStatus.BAD) {
            message += "Image is shifted too far from the reference frame\n"
        } else {
            if (translationX == HomographyValidationStatus.WARNING) {
                message += "Horizontal shift may affect alignment quality\n"
            }
            if (translationY == HomographyValidationStatus.WARNING) {
                message += "Vertical shift may affect alignment quality\n"
            }
        }

        return message.dropLast(1)
    }
}

enum class HomographyValidationStatus(val value: Int) {
    OK(0),
    WARNING(1),
    BAD(2);

    companion object {
        fun fromInt(value: Int) = HomographyValidationStatus.entries.first { it.value == value }
    }
}