package com.sedv.stackingcamera.stacking.settings.ui

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.LinearLayout
import androidx.core.view.isVisible
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.databinding.StackingSettingsSwitchBinding

class Switch @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : LinearLayout(context, attrs, defStyleAttr) {

    private val binding = StackingSettingsSwitchBinding.inflate(LayoutInflater.from(context), this, true)

    var onChangeCallback: ((Boolean) -> Unit)? = null

    var isChecked: Boolean
        get() = binding.switcher.isChecked
        set(value) { binding.switcher.isChecked = value }

    private var _currentActiveValue = true
    val isChanged get() = isChecked != _currentActiveValue

    init {
        context.theme.obtainStyledAttributes(
            attrs,
            R.styleable.StackingSettings,
            defStyleAttr, 0
        ).apply {
            try {
                binding.titleText.text = getString(R.styleable.StackingSettings_titleText)
                binding.descriptionText.text = getString(R.styleable.StackingSettings_descriptionText)
            } finally {
                recycle()
            }
        }

        if (binding.descriptionText.text.isNullOrBlank()) {
            binding.descriptionText.isVisible = false
        }

        binding.switcher.setOnCheckedChangeListener { _, isChecked ->
            onChangeCallback?.invoke(isChecked)
        }
    }

    fun init(startValue: Boolean) {
        isChecked = startValue
        _currentActiveValue = startValue
    }

    fun applyNewValue() {
        _currentActiveValue = isChecked
    }
}