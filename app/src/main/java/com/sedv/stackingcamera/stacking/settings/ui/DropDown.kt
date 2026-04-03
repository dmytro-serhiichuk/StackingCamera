package com.sedv.stackingcamera.stacking.settings.ui

import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.ArrayAdapter
import android.widget.AutoCompleteTextView
import android.widget.LinearLayout
import androidx.core.view.isVisible
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.databinding.StackingSettingsDropdownBinding

class DropDown @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : LinearLayout(context, attrs, defStyleAttr) {

    private val binding = StackingSettingsDropdownBinding.inflate(LayoutInflater.from(context), this, true)

    var value: String
        get() = binding.autoCompleteTextView.text.toString()
        set(value) { binding.autoCompleteTextView.setText(value, false) }

    private var _currentActiveValue = ""
    val isChanged get() = value != _currentActiveValue

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

    fun init(options: List<String>, startValue: String) {
        val adapter = ArrayAdapter(context, android.R.layout.simple_list_item_1, options)
        (binding.menuLayout.editText as? AutoCompleteTextView)?.setAdapter(adapter)
        binding.autoCompleteTextView.setText(startValue, false)
        _currentActiveValue = startValue
    }

    fun applyNewValue() {
        _currentActiveValue = value
    }
}