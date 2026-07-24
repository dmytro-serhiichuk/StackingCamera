package com.sedv.stackingcamera.main.camera.settings

import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.main.camera.CameraInfo

class EVProperty(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
) : BaseRangeProperty<Int>(
    cameraInfo,
    onSettingsManuallyChanged,
    "EV",
    0,
    false,
    false
) {

    private val evSteps = initEVSteps()

    override fun getDisplayValue(): String {
        val index = range.mainSteps.indexOf(value)
        return String.format("%.1f", evSteps[index])
    }

    override fun getIcon(): Int {
        return R.drawable.icon_ev
    }

    override fun initRange(): Range<Int> {
        val keySteps = initKeySteps()

        val steps = arrayListOf<Int>()
        var value = cameraInfo.evRange.lower
        while (value <= cameraInfo.evRange.upper) {
            steps.add(value)
            value += 1
        }

        return Range(steps, keySteps)
    }

    private fun initKeySteps(): List<Int> {
        val keySteps = arrayListOf<Int>()
        keySteps.add(cameraInfo.evRange.lower)
        keySteps.add(0)
        keySteps.add(cameraInfo.evRange.upper)
        return keySteps
    }

    private fun initEVSteps(): List<Double> {
        val stepValue = cameraInfo.evStep!!.toDouble()
        return (cameraInfo.evRange.lower..cameraInfo.evRange.upper).map { it * stepValue }
    }
}