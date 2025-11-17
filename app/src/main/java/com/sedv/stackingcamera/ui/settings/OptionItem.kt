package com.sedv.stackingcamera.ui.settings

import android.content.Context
import android.view.LayoutInflater
import android.widget.LinearLayout
import com.sedv.stackingcamera.databinding.SettingsPropertyRegularBinding

class OptionItem(
    context: Context,
    iconSrc: Int,
    label: String
) : LinearLayout(context) {

    private val binding = SettingsPropertyRegularBinding
        .inflate(LayoutInflater.from(context), this, true)

    init {
        binding.icon.setImageResource(iconSrc)
        binding.valueLabel.text = label
    }
}