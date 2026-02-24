package com.sedv.stackingcamera.main.settings.ui

import android.content.Context
import android.view.LayoutInflater
import android.widget.HorizontalScrollView
import android.widget.LinearLayout
import com.sedv.stackingcamera.main.camera.settings.BaseOptionsProperty
import com.sedv.stackingcamera.databinding.SettingsOptionsListBinding

class OptionsList(
    context: Context,
    val property: BaseOptionsProperty,
    private val onOptionSelected: ((Int) -> Unit)? = null
) : HorizontalScrollView(context) {

    private val binding = SettingsOptionsListBinding.inflate(LayoutInflater.from(context), this, true)

    private val options: Map<Int, OptionItem>
    private var selectedOption: OptionItem? = null

    init {
        options = HashMap<Int, OptionItem>()

        val layoutParams = LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.MATCH_PARENT
        )

        property.options.forEach { opt ->
            val optView = OptionItem(context, property.getIconFromOption(opt.key), opt.value)
            optView.layoutParams = layoutParams

            optView.setOnClickListener {
                selectedOption?.isSelected = false
                property.setValueWithNotifying(opt.key)
                selectedOption = optView
                selectedOption?.isSelected = true

                onOptionSelected?.invoke(opt.key)
            }

            if (opt.key == property.value) {
                selectedOption = optView
                selectedOption?.isSelected = true
                onOptionSelected?.invoke(opt.key)
            }

            binding.list.addView(optView)
            options[opt.key] = optView
        }
    }

    fun updateManually() {
        options.forEach { option ->
            if (option.key == property.value) {
                selectedOption?.isSelected = false
                selectedOption = option.value
                selectedOption?.isSelected = true
            }
        }
    }
}