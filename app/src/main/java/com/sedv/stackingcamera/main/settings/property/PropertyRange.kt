package com.sedv.stackingcamera.main.settings.property

import android.content.Context
import android.view.LayoutInflater
import android.widget.LinearLayout
import com.sedv.stackingcamera.main.camera.settings.BaseRangeProperty
import com.sedv.stackingcamera.databinding.SettingsPropertyRegularBinding

class PropertyRange<T>(
    context: Context,
    override val property: BaseRangeProperty<T>,
) : LinearLayout(context), IProperty where T : Number, T : Comparable<T> {

    private val binding = SettingsPropertyRegularBinding
        .inflate(LayoutInflater.from(context), this, true)

    init {
        binding.icon.setImageResource(property.getIcon())
        binding.valueLabel.text = property.getDisplayValue()
    }

    override fun handleValueChanged() {
        binding.valueLabel.text = property.getDisplayValue()
        requestLayout()
    }
}