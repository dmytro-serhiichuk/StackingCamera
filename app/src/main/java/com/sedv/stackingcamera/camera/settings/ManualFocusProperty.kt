package com.sedv.stackingcamera.camera.settings

import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.camera.CameraInfo
import kotlin.math.round

class ManualFocusProperty(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit
) : BaseRangeProperty<Float>(
    cameraInfo,
    onSettingsManuallyChanged,
    "Focus",
    round(cameraInfo.hyperfocalDistance * 100) / 100,
    true,
    false
) {

    override fun setValueWithNotifying(newValue: Float) {
        super.setValueWithNotifying(round(newValue * 100) / 100)
    }

    override fun getIcon(): Int {
        return R.drawable.icon_focus_manual
    }

    override fun setValueWithoutNotifying(newValue: Float) {
        super.setValueWithoutNotifying(round(newValue * 100) / 100)
    }

    override fun getDisplayValue(): String {
        return value.toString()
    }

    override fun initRange(): Range<Float> {
        val stepSize: Float = 1f/3f

        val steps = arrayListOf<Float>()
        var currentStep = 0f
        while (currentStep < cameraInfo.minFocusDistance) {
            val new = round(currentStep * 100) / 100
            if (steps.isEmpty() || steps.last() != new) {
                steps.add(new)
            }
            currentStep = currentStep + stepSize
        }
        steps.add(cameraInfo.minFocusDistance)

        return Range(steps, initKeySteps())
    }

    private fun initKeySteps(): List<Float> {
        val keySteps = arrayListOf<Float>()
        keySteps.add(0f)
        // TODO: main steps might not have hyperfocalDistance
        val hyperfocalDistance = round(cameraInfo.hyperfocalDistance * 100) / 100
        if (hyperfocalDistance != cameraInfo.minFocusDistance &&
            hyperfocalDistance != 0f) {
            keySteps.add(hyperfocalDistance)
        }
        keySteps.add(cameraInfo.minFocusDistance)
        return keySteps
    }
}