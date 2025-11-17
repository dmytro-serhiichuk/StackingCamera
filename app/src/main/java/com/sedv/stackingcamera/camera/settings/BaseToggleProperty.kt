package com.sedv.stackingcamera.camera.settings

import com.sedv.stackingcamera.camera.CameraInfo

abstract class BaseToggleProperty<T>(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
    name: String?,
    initValue: T
) : BaseSettingsProperty<T>(cameraInfo, onSettingsManuallyChanged, name, initValue) {
    abstract fun toggle()
}