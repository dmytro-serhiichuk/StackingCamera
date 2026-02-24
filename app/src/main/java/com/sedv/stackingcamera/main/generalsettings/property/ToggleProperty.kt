package com.sedv.stackingcamera.main.generalsettings.property

import com.sedv.stackingcamera.main.generalsettings.GeneralPropertyType

class ToggleProperty(
    type: GeneralPropertyType,
    onChanged: (BaseProperty<Boolean>) -> Unit,
    initValue: Boolean,
    val disabledIcon: Int
) : BaseProperty<Boolean>(type, onChanged, initValue) {

    fun toggle() {
        value = !value
    }

    val isActive: Boolean get() = value
}