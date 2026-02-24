package com.sedv.stackingcamera.main.camera.settings

import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.main.camera.CameraInfo
import kotlin.math.min

class BurstProperty(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
) : BaseRangeProperty<Int>(
    cameraInfo,
    onSettingsManuallyChanged,
    "Burst",
    1,
    false,
    false
) {

    override fun getDisplayValue(): String {
        return value.toString()
    }

    override fun initRange(): Range<Int> {
        val steps = arrayListOf<Int>()
        var value = 1
        while (value <= MAX_VALUE) {
            steps.add(value)
            value++
        }
        val keySteps = initKeySteps()
        return Range(steps, keySteps)
    }

    private fun initKeySteps(): List<Int> {
        val keySteps = arrayListOf(1)
        var value = min(5, MAX_VALUE)
        while (value <= MAX_VALUE) {
            keySteps.add(value)
            value += 5
        }
        return keySteps
    }

    override fun getIcon(): Int {
        return R.drawable.icon_burst
    }

    companion object {
        private const val MAX_VALUE = 500
    }
}