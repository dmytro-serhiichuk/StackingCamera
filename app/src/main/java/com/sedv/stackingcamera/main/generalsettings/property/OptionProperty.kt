package com.sedv.stackingcamera.main.generalsettings.property

import com.sedv.stackingcamera.main.generalsettings.GeneralPropertyType

class OptionProperty(
    type: GeneralPropertyType,
    onChanged: (BaseProperty<Int>) -> Unit,
    initValue: Int,
    val options: Map<Int, Int>
) : BaseProperty<Int>(type, onChanged, initValue)