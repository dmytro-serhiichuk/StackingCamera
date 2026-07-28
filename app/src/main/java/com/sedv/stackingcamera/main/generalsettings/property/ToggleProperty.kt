package com.sedv.stackingcamera.main.generalsettings.property

import com.sedv.stackingcamera.main.generalsettings.GeneralPropertyType

class ToggleProperty(
    type: GeneralPropertyType,
    onChanged: (BaseProperty<Boolean>) -> Unit,
    initValue: Boolean,
    val inactiveIcon: Int
) : BaseProperty<Boolean>(type, onChanged, initValue) {

    val activeIcon = type.drawable

    val isActive: Boolean get() = value

    override var value: Boolean
        get() = super.value && isAvailable
        set(value) { super.value = value }

    fun toggle() {
        value = !value
    }
}