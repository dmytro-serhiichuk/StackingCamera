package com.sedv.stackingcamera.main.camera.settings

import android.hardware.camera2.CameraCharacteristics
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.main.camera.CameraInfo

class FocusModesProperty (
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
) : BaseOptionsProperty(
    cameraInfo,
    onSettingsManuallyChanged,
    "Focus",
    CameraCharacteristics.CONTROL_AF_MODE_CONTINUOUS_PICTURE
) {
    override fun initOptions(): Map<Int, String> {
        val options = HashMap<Int, String>()
        cameraInfo.afModes.forEach {
            val name = when(it) {
                CameraCharacteristics.CONTROL_AF_MODE_OFF -> {
                    if (cameraInfo.minFocusDistance != 0.0f) "Manual"
                    else null
                }
                CameraCharacteristics.CONTROL_AF_MODE_AUTO -> "Fixed"
                CameraCharacteristics.CONTROL_AF_MODE_CONTINUOUS_PICTURE -> "Auto"
//                CameraCharacteristics.CONTROL_AF_MODE_CONTINUOUS_VIDEO -> "Auto"
                CameraCharacteristics.CONTROL_AF_MODE_EDOF -> "EDOF"
                else -> null
            }
            if (name != null) options.put(it, name)
        }
        return options
    }

    override fun getDisplayValue(): String {
        return options[value] ?: "NULL"
    }

    fun switchToManual() {
        setValueWithNotifying(CameraCharacteristics.CONTROL_AF_MODE_OFF)
    }

    override fun getIcon(): Int {
        return getIconFromOption(value)
    }

    override fun getIconFromOption(key: Int): Int {
        return when (key) {
            CameraCharacteristics.CONTROL_AF_MODE_OFF -> R.drawable.icon_focus_manual
            CameraCharacteristics.CONTROL_AF_MODE_AUTO -> R.drawable.icon_focus_fixed
            CameraCharacteristics.CONTROL_AF_MODE_CONTINUOUS_PICTURE -> R.drawable.icon_focus_auto
            CameraCharacteristics.CONTROL_AF_MODE_CONTINUOUS_VIDEO -> R.drawable.icon_focus_auto
            CameraCharacteristics.CONTROL_AF_MODE_EDOF -> R.drawable.icon_focus_edof
            else -> throw RuntimeException("Invalid focus state")
        }
    }
}