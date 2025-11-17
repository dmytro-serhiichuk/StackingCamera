package com.sedv.stackingcamera.camera.settings

import android.graphics.ImageFormat
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.camera.CameraInfo

class FormatProperty(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
) : BaseToggleProperty<CameraOutputFormat>(
    cameraInfo,
    onSettingsManuallyChanged,
    null,
    CameraOutputFormat.JPEG
) {

    override fun getDisplayValue(): String {
        return value.displayName
    }

    override fun toggle() {
        if (value == CameraOutputFormat.JPEG) setValueWithoutNotifying(CameraOutputFormat.RAW)
        else setValueWithoutNotifying(CameraOutputFormat.JPEG)
    }

    override fun getIcon(): Int {
        return value.icon
    }
}

enum class CameraOutputFormat(val value: Int, val ext: String, val mimeType: String, val displayName: String, val icon: Int) {
    JPEG(ImageFormat.JPEG, ".jpg", "image/jpeg", "JPEG", R.drawable.icon_format_jpeg),
    RAW(ImageFormat.RAW_SENSOR, ".dng", "image/raw", "RAW", R.drawable.icon_format_raw)
}