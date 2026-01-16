package com.sedv.stackingcamera.stacking.settings

import android.util.Range

abstract class Property<T>(
    val name: String,
    val defaultValue: T,
    open var value: T = defaultValue
)

class RangedProperty<T : Comparable<T>>(
    name: String,
    defaultValue: T,
    val range: Range<T>
) : Property<T>(name, defaultValue)

class BoolProperty(
    name: String,
    defaultValue: Boolean,
) : Property<Boolean>(name, defaultValue)
