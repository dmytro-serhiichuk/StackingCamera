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
)

enum class HomographyValidationStatus(val value: Int) {
    OK(0),
    WARNING(1),
    BAD(2);

    companion object {
        fun fromInt(value: Int) = HomographyValidationStatus.entries.first { it.value == value }
    }
}