package com.sedv.stackingcamera.main.camera.settings

import com.sedv.stackingcamera.main.camera.CameraInfo

abstract class BaseOptionsProperty(
    cameraInfo: CameraInfo,
    onSettingsManuallyChanged: () -> Unit,
    name: String?,
    initValue: Int
) : BaseSettingsProperty<Int>(cameraInfo, onSettingsManuallyChanged, name, initValue) {
    val options: Map<Int, String> = initOptions()

    abstract protected fun initOptions(): Map<Int, String>

    abstract fun getIconFromOption(key: Int): Int
}