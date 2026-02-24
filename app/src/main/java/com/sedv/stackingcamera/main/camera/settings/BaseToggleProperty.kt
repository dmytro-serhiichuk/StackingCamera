package com.sedv.stackingcamera.main.camera.settings

import com.sedv.stackingcamera.main.camera.CameraInfo

abstract class BaseToggleProperty<T>(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
    name: String?,
    initValue: T
) : BaseSettingsProperty<T>(cameraInfo, onSettingsManuallyChanged, name, initValue) {
    abstract fun toggle()
}