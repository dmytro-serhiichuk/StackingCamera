package com.sedv.stackingcamera.settings

class OptionProperty(
    type: GeneralPropertyType,
    onChanged: (BaseProperty<Int>) -> Unit,
    initValue: Int,
    val options: Map<Int, Int>
) : BaseProperty<Int>(type, onChanged, initValue)