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
    fun hasWarningStatus(): Boolean {
        return props.any { it == BitmapValidationStatus.WARNING }
    }

    fun hasBadStatus(): Boolean {
        return props.any { it == BitmapValidationStatus.BAD } || !isConvex || mirrored
    }

    fun getMessage(): String {
        if (!isConvex || mirrored) {
            return "Image can not be aligned with the reference image"
        }

        return buildList {
            addStatusMessage(
                perspective,
                "Perspective distortion is too strong for reliable alignment",
                "Strong perspective differences may cause misalignment of objects at different depths"
            )
            addStatusMessage(
                anisotropy,
                "Non-uniform image scaling is too strong",
                "Uneven scaling may reduce alignment accuracy"
            )
            addStatusMessage(
                shear,
                "Image is too strongly skewed",
                "Image skew may slightly reduce output quality"
            )
            addStatusMessage(
                scale,
                "Image scale difference is too large",
                "Significant scaling may reduce image detail"
            )
            addTranslationMessages()
            addStatusMessage(
                inliersPercentage,
                "Too few reliable matches were found",
                "A low proportion of reliable matches may indicate poor overall matching quality"
            )
            addStatusMessage(
                inliersNumber,
                "Too few matches were found between the images",
                "A small number of matches may make precise alignment impossible"
            )
            addStatusMessage(
                evenDistribution,
                "Matches cover only a small part of the image",
                "Matches are unevenly distributed, which may cause poor alignment in some areas of the image"
            )
        }.joinToString("\n")
    }

    private fun MutableList<String>.addStatusMessage(
        status: BitmapValidationStatus,
        bad: String,
        warning: String
    ) {
        when (status) {
            BitmapValidationStatus.BAD     -> add(bad)
            BitmapValidationStatus.WARNING -> add(warning)
            else                           -> Unit
        }
    }

    private fun MutableList<String>.addTranslationMessages() {
        if (translationX == BitmapValidationStatus.BAD || translationY == BitmapValidationStatus.BAD) {
            add("Image is shifted too far from the reference frame")
            return
        }
        if (translationX == BitmapValidationStatus.WARNING) add("Horizontal shift may affect alignment quality")
        if (translationY == BitmapValidationStatus.WARNING) add("Vertical shift may affect alignment quality")
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