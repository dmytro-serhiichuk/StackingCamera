package com.sedv.stackingcamera.ui.settings

import com.sedv.stackingcamera.camera.settings.BaseSettingsProperty

interface IProperty {
    val property: BaseSettingsProperty<*>

    fun handleValueChanged()
}