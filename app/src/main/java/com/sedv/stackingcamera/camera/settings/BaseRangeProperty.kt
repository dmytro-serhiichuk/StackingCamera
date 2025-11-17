package com.sedv.stackingcamera.camera.settings

import com.sedv.stackingcamera.camera.CameraInfo

abstract class BaseRangeProperty<T>(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
    name: String?,
    initValue: T,
    val hasAutoMode: Boolean,
    val hasSwitcher: Boolean = true
) : BaseSettingsProperty<T>(
    cameraInfo,
    onSettingsManuallyChanged,
    name,
    initValue
) where T : Number, T : Comparable<T> {

    val range: Range<T> = initRange()
    protected var _isInAutoMode: Boolean = hasAutoMode
    var isInAutoMode
        get() = _isInAutoMode
        set(value) {
            if (hasAutoMode) {
                _isInAutoMode = value
                onSettingsManuallyChanged()
            }
        }

    fun setAutoModeWithoutNotifying(newValue: Boolean) {
        _isInAutoMode = newValue
    }

    abstract protected fun initRange(): Range<T>

    fun getRangeIndexFromValue(target: T): Int {
        if (range.mainSteps.isEmpty()) return 0

        val index = range.mainSteps.binarySearch(target)

        val rangeSize = range.mainSteps.size

        return when {
            index >= 0 -> index
            else -> {
                val insertionPoint = -(index + 1)
                when {
                    insertionPoint == 0 -> 0
                    insertionPoint >= rangeSize -> rangeSize - 1
                    else -> {
                        val targetDouble = target.toDouble()
                        val leftDistance = targetDouble - range.mainSteps[insertionPoint - 1].toDouble()
                        val rightDistance = range.mainSteps[insertionPoint].toDouble() - targetDouble
                        if (leftDistance <= rightDistance) insertionPoint - 1 else insertionPoint
                    }
                }
            }
        }
    }
}

data class Range<T>(
    val mainSteps: List<T> = emptyList(),
    val keySteps: List<T> = emptyList()
) where T : Number, T : Comparable<T>