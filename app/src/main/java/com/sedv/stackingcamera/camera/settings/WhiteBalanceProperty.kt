package com.sedv.stackingcamera.camera.settings

import android.hardware.camera2.CameraCharacteristics
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.camera.CameraInfo

class WhiteBalanceProperty(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
) : BaseOptionsProperty(
    cameraInfo,
    onSettingsManuallyChanged,
    "WB",
    CameraCharacteristics.CONTROL_AWB_MODE_AUTO
) {

    override fun getDisplayValue(): String {
        return options[value] ?: "NULL"
    }

    override fun initOptions(): Map<Int, String> {
        val options = HashMap<Int, String>()
        cameraInfo.awbModes.forEach {
            val name = when(it) {
                CameraCharacteristics.CONTROL_AWB_MODE_OFF -> "OFF"
                CameraCharacteristics.CONTROL_AWB_MODE_AUTO -> "Auto"
                CameraCharacteristics.CONTROL_AWB_MODE_INCANDESCENT -> "Incandescent"
                CameraCharacteristics.CONTROL_AWB_MODE_FLUORESCENT -> "Fluorescent"
                CameraCharacteristics.CONTROL_AWB_MODE_WARM_FLUORESCENT -> "Warm Fluorescent"
                CameraCharacteristics.CONTROL_AWB_MODE_DAYLIGHT -> "Daylight"
                CameraCharacteristics.CONTROL_AWB_MODE_CLOUDY_DAYLIGHT -> "Cloudy"
                CameraCharacteristics.CONTROL_AWB_MODE_TWILIGHT -> "Twilight"
                CameraCharacteristics.CONTROL_AWB_MODE_SHADE -> "Shade"
                else -> null
            }
            if (name != null) options.put(it, name)
        }
        return options
    }

    override fun getIconFromOption(key: Int): Int {
        return when (key) {
            CameraCharacteristics.CONTROL_AWB_MODE_OFF -> R.drawable.icon_off
            CameraCharacteristics.CONTROL_AWB_MODE_AUTO -> R.drawable.icon_wb_awb
            CameraCharacteristics.CONTROL_AWB_MODE_INCANDESCENT -> R.drawable.icon_wb_incandescent
            CameraCharacteristics.CONTROL_AWB_MODE_FLUORESCENT -> R.drawable.icon_wb_fluorescent
            CameraCharacteristics.CONTROL_AWB_MODE_WARM_FLUORESCENT -> R.drawable.icon_wb_fluorescent
            CameraCharacteristics.CONTROL_AWB_MODE_DAYLIGHT -> R.drawable.icon_wb_daylight
            CameraCharacteristics.CONTROL_AWB_MODE_CLOUDY_DAYLIGHT -> R.drawable.icon_wb_cloudy
            CameraCharacteristics.CONTROL_AWB_MODE_TWILIGHT -> R.drawable.icon_wb_twilight
            CameraCharacteristics.CONTROL_AWB_MODE_SHADE -> R.drawable.icon_wb_shade
            else -> R.drawable.icon_wb
        }
    }

    override fun getIcon(): Int {
        return R.drawable.icon_wb
    }
}