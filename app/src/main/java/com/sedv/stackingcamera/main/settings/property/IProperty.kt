package com.sedv.stackingcamera.main.settings.property

import com.sedv.stackingcamera.main.camera.settings.BaseSettingsProperty

interface IProperty {
    val property: BaseSettingsProperty<*>

    fun handleValueChanged()
}