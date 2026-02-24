package com.sedv.stackingcamera.main.generalsettings

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.widget.FrameLayout
import android.widget.HorizontalScrollView
import android.widget.ImageView
import android.widget.LinearLayout
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.main.CameraViewModel
import com.sedv.stackingcamera.main.camera.CameraState
import com.sedv.stackingcamera.main.generalsettings.property.OptionProperty
import com.sedv.stackingcamera.main.generalsettings.property.ToggleProperty

class GeneralSettingsController(
    private val context: Context,
    private val viewModel: CameraViewModel,
    private val mainScrollContainer: HorizontalScrollView,
    private val bottomScrollView: HorizontalScrollView
) {
    private val propList: LinearLayout
    private val additionalList: LinearLayout

    private var selectedProperty: ImageView? = null
    private var selectedOption: ImageView? = null

    private var _isOpen = false
    val isOpen get() = _isOpen

    private val layoutParams = FrameLayout.LayoutParams(
        FrameLayout.LayoutParams.WRAP_CONTENT,
        FrameLayout.LayoutParams.MATCH_PARENT
    )

    init {
        propList = LinearLayout(context).also {
            it.layoutParams = layoutParams
            it.orientation = LinearLayout.HORIZONTAL
        }
        mainScrollContainer.addView(propList)

        additionalList = LinearLayout(context).also {
            it.layoutParams = layoutParams
            it.orientation = LinearLayout.HORIZONTAL
        }
        bottomScrollView.addView(additionalList)

        GeneralSettings.properties.forEach { property ->
            val view = LayoutInflater.from(context).inflate(R.layout.general_settings_list_item, propList, false) as ImageView
            if (property is ToggleProperty) {
                view.setImageResource(property.disabledIcon)
            } else if (property is OptionProperty) {
                view.setImageResource(property.type.drawable)
            }

            view.setOnClickListener {
                if (selectedProperty == view) {
                    close()
                } else {
                    close()

                    if (property is OptionProperty) {
                        if (property.type == GeneralPropertyType.FRAME_SIZE &&
                            viewModel.activeCamera.currentState != CameraState.OPENED) {
                            return@setOnClickListener
                        }
                        selectedProperty = view
                        selectedProperty?.isSelected = true
                        openBottomContainer(property)
                    } else if (property is ToggleProperty) {
                        property.toggle()
                        if (property.value) {
                            view.setImageResource(property.type.drawable)
                        } else {
                            view.setImageResource(property.disabledIcon)
                        }
                    }
                }
            }

            propList.addView(view)
        }
    }

    private fun openBottomContainer(prop: OptionProperty) {
        _isOpen = true

        prop.options.forEach { option ->
            val view = LayoutInflater.from(context).inflate(R.layout.general_settings_list_item, additionalList, false) as ImageView
            view.setImageResource(option.value)

            view.setOnClickListener {
                selectedOption?.isSelected = false
                selectedOption = view
                selectedOption?.isSelected = true

                prop.value = option.key
                selectedProperty?.setImageResource(option.value)
            }

            if (option.key == prop.value) {
                selectedOption = view
                selectedOption?.isSelected = true
            }

            additionalList.addView(view)
        }

        bottomScrollView.visibility = View.VISIBLE
    }

    fun close() {
        _isOpen = false
        selectedProperty?.isSelected = false
        selectedProperty = null
        additionalList.removeAllViews()
        bottomScrollView.visibility = View.GONE
    }
}