package com.sedv.stackingcamera.main.camera

import android.content.res.Resources
import android.hardware.camera2.CameraCharacteristics
import android.util.Range
import android.util.Rational
import android.util.Size
import android.util.SizeF
import kotlin.math.atan
import kotlin.math.pow
import kotlin.math.roundToInt
import kotlin.math.sqrt

data class CameraInfo(
    val cameraId: String,
    var cameraType: CameraType = CameraType.UNDEFINED,
    var cameraLabel: String = "ID: $cameraId",
    val facing: Int,
    val sensorOrientation: Int,
    val supportedFormats: List<CameraPhotoFormat>,
    val supportedPreviewSizes: List<Size>,
    val aeFpsRanges: List<Range<Int>>,
    val highSpeedFpsRanges: List<Range<Int>>,
    val hasFlash: Boolean,
    val maxZoom: Float,
    val focalLengths: FloatArray,
    val apertures: FloatArray,
    val minFocusDistance: Float,
    val hyperfocalDistance: Float,
    val oisModes: IntArray,
    val hasManualSensor: Boolean,
    val hasRawCapture: Boolean,
    val hasBurst: Boolean,
    val hasYuvReprocessing: Boolean,
    val physicalSize: SizeF,
    val afModes: Set<Int>,
    val hasManualFocus: Boolean,
    val awbModes: IntArray,
    val arraySize: Size,
    val exposureRange: Range<Long>,
    val isoRange: Range<Int>,
    val evRange: Range<Int>,
    val evStep: Rational?,
    val colorFilter: Int,
    val afRegions: Int,
    val aeRegions: Int,
    val awbRegions: Int,
    val physicalCameraIds: Set<String>
) {

    var supportSemiAutoExposure: Boolean = true

    val isFront: Boolean get() = facing == CameraCharacteristics.LENS_FACING_FRONT

    val diagonal =
        sqrt(physicalSize.width.toDouble().pow(2.0) + physicalSize.height.toDouble().pow(2.0))
    val fov = Math.toDegrees(2.0 * atan((diagonal / (2.0 * focalLengths[0].toDouble())))).toFloat()
    val cropFactor = 43.27 / diagonal

    val equivalentFocalLengths: FloatArray
        get() {
            return focalLengths.map {
                it * cropFactor.toFloat()
            }.toFloatArray()
        }

    val isLogicalCamera: Boolean get() = physicalCameraIds.isNotEmpty()

    fun getPreviewSizeByAspectRatio(ratio: Double): Size {
        val factor = 1000.0

        var previewSize = supportedPreviewSizes.getOrNull(0) ?: Size(0, 0)

        val ratioInt = (ratio * factor).roundToInt() / factor

        for (size in supportedPreviewSizes) {
            val currentRatio = (size.width.toDouble() / size.height * factor).roundToInt() / factor
            if (currentRatio == ratioInt) {
                val rotated = getRotatedSize(size)
                if (rotated.width <= MAX_WIDTH) {
                    previewSize = rotated
                    break
                }
            }
        }
        return previewSize
    }

    fun getRotatedSize(size: Size): Size {
        return when(sensorOrientation) {
            0, 180 -> size
            else -> Size(size.height, size.width)
        }
    }
    fun getRotatedPoint(point: Pair<Float, Float>): Pair<Float, Float> {
        val (x, y) = point

        return if (facing == CameraCharacteristics.LENS_FACING_BACK) {
            when (sensorOrientation) {
                90 -> Pair(y, 1f - x)
                180 -> Pair(x, y)
                270 -> Pair(1f - y, x)
                else -> Pair(1f - x, 1f - y)
            }
        } else {
            when (sensorOrientation) {
                90 -> Pair(1f - y, x)
                180 -> Pair(1f - x, 1f - y)
                270 -> Pair(y, 1f - x)
                else -> Pair(x, y)
            }
        }
    }

    override fun toString(): String {
        return "Camera ID: $cameraId\n" +
                "Camera Label: $cameraLabel\n" +
                "Is Front: $isFront\n" +
                "Orientation: $sensorOrientation\n" +
                "Supported Formats: $supportedFormats\n" +
                "AE FPS Ranges: $aeFpsRanges\n" +
                "High-Speed FPS Ranges: $highSpeedFpsRanges\n" +
                "Has Flash: $hasFlash\n" +
                "Max Zoom: $maxZoom\n" +
                "Focal Lengths: ${focalLengths.toList()}\n" +
                "EFL: ${equivalentFocalLengths.toList()}\n" +
                "Apertures: ${apertures.toList()}\n" +
                "FOV: $fov\n" +
                "Crop Factor: $cropFactor\n" +
                "Min Focus Distance: $minFocusDistance\n" +
                "Hyperfocal Distance: $hyperfocalDistance\n" +
                "OIS Modes: ${oisModes.toList()}\n" +
                "Has Manual Sensor: $hasManualSensor\n" +
                "Has Raw: $hasRawCapture\n" +
                "Has Burst: $hasBurst\n" +
                "Has YUV Reproc: $hasYuvReprocessing\n" +
                "Physical Size: $physicalSize\n" +
                "Diagonal: $diagonal\n" +
                "AF Modes: $afModes\n" +
                "Has Manual Focus: $hasManualFocus\n" +
                "AWB Modes: ${awbModes.toList()}\n" +
                "Sensor Resolution: $arraySize\n" +
                "Exposure Time Range: $exposureRange\n" +
                "ISO Range: $isoRange\n" +
                "EV Range: $evRange\n" +
                "EV Step: $evStep\n" +
                "Color Filter: $colorFilter\n" +
                "AF Regions: $afRegions\n" +
                "AE Regions: $aeRegions\n" +
                "AWB Regions: $awbRegions\n" +
                "Support Semi-Auto Exposure: $supportSemiAutoExposure\n" +
                "Physical Ids: $physicalCameraIds"
    }

    companion object {
        private val MAX_WIDTH = Resources.getSystem().displayMetrics.widthPixels
    }
}
