package com.sedv.stackingcamera.main.generalsettings.property

import com.sedv.stackingcamera.main.generalsettings.GeneralPropertyType

abstract class BaseProperty<T>(
    val type: GeneralPropertyType,
    private val onChanged: (BaseProperty<T>) -> Unit,
    initValue: T
) {
    open var value: T = initValue
        set(value) {
            if (value != field) {
                field = value
                onChanged(this)
            }
        }

    var isAvailable: Boolean = true
        set(value) {
            field = value
            onChanged(this)
        }

    override fun equals(other: Any?): Boolean {
        val prop = other as? BaseProperty<*>
        return if (prop != null) {
            type == prop.type
        } else {
            false
        }
    }
}