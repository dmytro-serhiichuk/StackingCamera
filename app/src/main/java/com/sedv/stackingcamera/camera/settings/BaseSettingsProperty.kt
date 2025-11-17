package com.sedv.stackingcamera.camera.settings

import com.sedv.stackingcamera.camera.CameraInfo

abstract class BaseSettingsProperty<T>(
    val cameraInfo: CameraInfo,
    val onSettingsManuallyChanged: () -> Unit,
    val name: String?,
    initValue: T
) {
    protected var _value: T = initValue
    var value: T
        get() = _value
        protected set(value) { _value = value }

    abstract fun getDisplayValue(): String

    open fun setValueWithNotifying(newValue: T) {
        _value = newValue
        onSettingsManuallyChanged()
    }
    open fun setValueWithoutNotifying(newValue: T) {
        _value = newValue
    }

    abstract fun getIcon(): Int
}