package com.sedv.stackingcamera.ui.settings

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.FrameLayout
import com.sedv.stackingcamera.camera.settings.BaseToggleProperty
import com.sedv.stackingcamera.databinding.SettingsPropertyToggleBinding

class PropertyToggle<T>(
    context: Context,
    override val property: BaseToggleProperty<T>,
) : FrameLayout(context), IProperty {

    private val binding = SettingsPropertyToggleBinding
        .inflate(LayoutInflater.from(context), this, true)

    init {
        binding.icon.setImageResource(property.getIcon())
    }

    override fun handleValueChanged() {
        binding.icon.setImageResource(property.getIcon())
    }
}