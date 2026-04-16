package com.sedv.stackingcamera.main.settings.property

import android.content.Context
import android.view.LayoutInflater
import android.widget.LinearLayout
import com.sedv.stackingcamera.main.camera.settings.BaseOptionsProperty
import com.sedv.stackingcamera.databinding.SettingsPropertyRegularBinding

class PropertyOptions(
    context: Context,
    override val property: BaseOptionsProperty,
) : LinearLayout(context), IProperty {

    private val binding = SettingsPropertyRegularBinding
        .inflate(LayoutInflater.from(context), this, true)

    init {
        binding.icon.setImageResource(property.getIcon())
        binding.valueLabel.text = property.getDisplayValue()
    }
    override fun handleValueChanged() {
        binding.icon.setImageResource(property.getIcon())
        binding.valueLabel.text = property.getDisplayValue()
    }
}