package com.sedv.stackingcamera.camera.settings

import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.camera.CameraInfo
import kotlin.math.roundToInt

class ShutterProperty(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
) : BaseRangeProperty<Long>(
    cameraInfo,
    onSettingsManuallyChanged,
    "Shutter",
    cameraInfo.exposureRange.lower,
    true
) {

    override fun getDisplayValue(): String {
        val divider = getExposureTimeDivider()
        val shutterStr = if (divider > 1.0) {
            "1/${divider.toInt()} (s)"
        } else {
            "${(1.0 / divider * 10).roundToInt() / 10} (s)"
        }

        return if (isInAutoMode) "Ⓐ $shutterStr"
        else shutterStr
    }

    override fun getIcon(): Int {
        return R.drawable.icon_shutter_speed
    }

    override fun initRange(): Range<Long> {
        val stepsBetweenKeySteps = 10L

        val keySteps = initKeySteps()

        val steps = arrayListOf<Long>()
        for (i in 0 until keySteps.size - 1) {
            var current = keySteps[i]
            val next = keySteps[i + 1]

            val diff = next - current
            val stepSize = (next - current) / stepsBetweenKeySteps

            while (current < next) {
                steps.add(current)
                current += stepSize
            }
        }
        steps.add(keySteps.last())

        return Range(steps, keySteps)
    }

    private fun initKeySteps(): List<Long> {
        val keySteps = arrayListOf<Long>()

        val startValue = 1_000_000_000L

        var value = startValue

        while (value < cameraInfo.exposureRange.upper) {
            keySteps.add(value)
            value *= 2
        }
        keySteps.add(cameraInfo.exposureRange.upper)

        value = startValue / 2
        while (value > cameraInfo.exposureRange.lower) {
            keySteps.add(0, value)
            value /= 2
        }
        keySteps.add(0, cameraInfo.exposureRange.lower)

        return keySteps
    }

    fun getExposureTimeDivider(): Double {
        return 1.0 / (value / 1_000_000_000.0)
    }
}