package com.sedv.stackingcamera.stacking.settings.ui

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.LinearLayout
import androidx.core.view.isVisible
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.databinding.StackingSettingsSliderBinding

class Slider @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : LinearLayout(context, attrs, defStyleAttr) {

    private val binding = StackingSettingsSliderBinding.inflate(LayoutInflater.from(context), this, true)

    private lateinit var formatValue: (Float) -> String

    var value: Float
        get() = binding.slider.value
        set(value) { binding.slider.value = value }

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
    }

    fun init(from: Float, to: Float, startValue: Float, step: Float, formatFunc: (Float) -> String) {
        formatValue = formatFunc

        binding.slider.valueFrom = from
        binding.slider.valueTo   = to
        binding.slider.stepSize  = step
        binding.slider.value     = startValue

        binding.sliderValue.text = formatValue(startValue)

        binding.slider.addOnChangeListener { _, value, _ ->
            binding.sliderValue.text = formatValue(value)
        }

        if ((to - from) / step > MAX_VISIBLE_STEPS) {
            binding.slider.isTickVisible = false
        }
    }

    companion object {
        private const val MAX_VISIBLE_STEPS = 20
    }

}