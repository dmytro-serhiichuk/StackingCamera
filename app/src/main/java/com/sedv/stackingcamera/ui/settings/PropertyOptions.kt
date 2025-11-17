package com.sedv.stackingcamera.ui.settings

import android.content.Context
import android.view.LayoutInflater
import android.widget.LinearLayout
import androidx.constraintlayout.widget.ConstraintLayout
import com.sedv.stackingcamera.camera.settings.BaseOptionsProperty
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

    // TODO: fix element width
    override fun handleValueChanged() {
        binding.icon.setImageResource(property.getIcon())
        binding.valueLabel.text = property.getDisplayValue()
    }
}