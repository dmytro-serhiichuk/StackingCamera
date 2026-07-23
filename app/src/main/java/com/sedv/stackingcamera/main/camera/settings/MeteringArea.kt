package com.sedv.stackingcamera.main.camera.settings

import android.hardware.camera2.params.MeteringRectangle
import android.util.Size
import com.sedv.stackingcamera.main.camera.CameraInfo

class MeteringArea(
    val cameraInfo: CameraInfo,
    val onTriggerUpdated: (Boolean) -> Unit,
    val onSettingsManuallyChanged: () -> Unit,
    val supportAE: Boolean,
    val supportAWB: Boolean
) {
    var onFocusStateUpdated: ((Boolean) -> Unit)? = null
    var triggerId = 0

    private var value: MeteringRectangle? = null

    val regions = arrayOf(value)

    fun setArea(x: Float, y: Float, previewSize: Size) {
        val normalizedX = x / previewSize.width.toFloat()
        val normalizedY = y / previewSize.height.toFloat()

        val (xR, yR) = cameraInfo.getRotatedPoint(Pair(normalizedX, normalizedY))

        val focusX = (xR * cameraInfo.arraySize.width).toInt()
        val focusY = (yR * cameraInfo.arraySize.height).toInt()

        val areaSize = (cameraInfo.arraySize.height * WIDTH_FRACTION).toInt()
        val clampedLeft = (focusX - areaSize / 2).coerceIn(
            0, cameraInfo.arraySize.width - areaSize
        )
        val clampedTop = (focusY - areaSize / 2).coerceIn(
            0, cameraInfo.arraySize.height - areaSize
        )

        value = MeteringRectangle(
            clampedLeft,
            clampedTop,
            areaSize,
            areaSize,
            MeteringRectangle.METERING_WEIGHT_MAX - 1
        )
        regions[0] = value

        onTriggerUpdated(true)

        value = null
        regions[0] = null
    }

    fun resetState() {
        onTriggerUpdated(false)
    }

    companion object {
        const val WIDTH_FRACTION = 0.18f
    }
}