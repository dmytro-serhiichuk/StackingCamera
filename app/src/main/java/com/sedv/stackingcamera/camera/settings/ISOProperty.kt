package com.sedv.stackingcamera.camera.settings

import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.camera.CameraInfo
import kotlin.math.max

class ISOProperty(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
) : BaseRangeProperty<Int>(
    cameraInfo,
    onSettingsManuallyChanged,
    "ISO",
    cameraInfo.isoRange.lower,
    true
) {

    override fun getDisplayValue(): String {
        return if (isInAutoMode) "Ⓐ $value"
        else value.toString()
    }

    override fun getIcon(): Int {
        return R.drawable.icon_iso
    }

    override fun initRange(): Range<Int> {
        val stepsBetweenKeySteps = 10
        val minStepSize = 20

        val keySteps = initKeySteps()

        val steps = arrayListOf<Int>()
        for (i in 0 until keySteps.size - 1) {
            var current = keySteps[i]
            val next = keySteps[i + 1]

            val diff = next - current
            val stepSize = max(diff / stepsBetweenKeySteps, minStepSize)

            while (current < next) {
                steps.add(current)
                current += stepSize
            }
        }
        steps.add(keySteps.last())

        return Range(steps, keySteps)
    }

    private fun initKeySteps(): List<Int> {
        val keySteps = arrayListOf<Int>()
        var lower = cameraInfo.isoRange.lower
        while (lower <= cameraInfo.isoRange.upper) {
            keySteps.add(lower)
            lower *= 2
        }
        return keySteps
    }
}