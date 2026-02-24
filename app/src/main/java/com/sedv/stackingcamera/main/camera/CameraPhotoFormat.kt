package com.sedv.stackingcamera.main.camera

import android.util.Size
import kotlin.math.roundToInt

data class CameraPhotoFormat(
    val format: Int,
    val supportedResolutions: List<Size>
) {
    fun getHighestResolutionByAspectRation(ratio: Double): Size? {
        val rationInt = (ratio * 1000).roundToInt()
        for (resolution in supportedResolutions) {
            val r = resolution.width.toDouble() / resolution.height
            val rInt = (r * 1000).roundToInt()
            if (rationInt == rInt) {
                return resolution
            }
        }
        return null
    }

    override fun toString(): String {
        return "Format: $format\nResolutions: $supportedResolutions"
    }
}