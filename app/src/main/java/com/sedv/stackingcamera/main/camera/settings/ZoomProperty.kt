package com.sedv.stackingcamera.main.camera.settings

import android.graphics.Rect
import com.sedv.stackingcamera.main.camera.CameraInfo
import kotlin.math.round

class ZoomProperty(
    val cameraInfo: CameraInfo,
    val onSettingsManuallyChanged: () -> Unit
) {
    private val centerX = cameraInfo.arraySize.width / 2
    private val centerY = cameraInfo.arraySize.height / 2

    var value: Float = 1.0f
        get() = field
        set(value) {
            val newValue = round(value.coerceIn(1.0f, cameraInfo.maxZoom) * 10) / 10
            if (field != newValue) {
                field = newValue

                val halfWidth = (cameraInfo.arraySize.width / field / 2).toInt()
                val halfHeight = (cameraInfo.arraySize.height / field / 2).toInt()

                _rect = Rect(
                    centerX - halfWidth,
                    centerY - halfHeight,
                    centerX + halfWidth,
                    centerY + halfHeight
                )

                onSettingsManuallyChanged()
            }
        }

    private var _rect = Rect(0, 0, cameraInfo.arraySize.width, cameraInfo.arraySize.height)
    val rect get() = _rect
}