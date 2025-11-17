package com.sedv.stackingcamera.ui.settings

import android.content.Context
import android.hardware.camera2.CameraCharacteristics
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.View.INVISIBLE
import android.view.View.VISIBLE
import android.widget.FrameLayout
import android.widget.LinearLayout
import androidx.appcompat.widget.AppCompatButton
import androidx.core.view.forEach
import androidx.core.view.isVisible
import com.sedv.stackingcamera.CameraViewModel
import com.sedv.stackingcamera.camera.settings.BaseOptionsProperty
import com.sedv.stackingcamera.camera.settings.BaseRangeProperty
import com.sedv.stackingcamera.camera.settings.BaseToggleProperty
import com.sedv.stackingcamera.camera.settings.FocusModesProperty
import com.sedv.stackingcamera.camera.settings.ISOProperty
import com.sedv.stackingcamera.camera.settings.ManualFocusProperty
import com.sedv.stackingcamera.camera.settings.ShutterProperty
import com.sedv.stackingcamera.ui.PreviewBottomContainerManager

class SettingsController(
    private val context: Context,
    private val viewModel: CameraViewModel,
    private val previewBottomContainerManager: PreviewBottomContainerManager,
    private val settingsList: LinearLayout,
    private val sliderWrapperContainer: FrameLayout,
    private val autoModeToggle: AppCompatButton
) {
    private val handler = Handler(Looper.getMainLooper())

    private var _isOpen = false
    val isOpen get() = _isOpen

    private var selectedProperty: IProperty? = null

    private var slider: SliderView? = null
    private var optionsList: OptionsList? = null

    init {
        viewModel.onCameraSwitched += ::handleCameraSwitched
        viewModel.onProgramReady += ::handleCameraSwitched

        val settings = viewModel.activeCamera.cameraSettings

        autoModeToggle.setOnClickListener {
            selectedProperty?.property?.let { prop ->
                if (prop is BaseRangeProperty<*> && prop.hasAutoMode) {
                    val newValue = !prop.isInAutoMode
                    if ((prop is ISOProperty || prop is ShutterProperty) && !settings.cameraInfo.supportSemiAutoExposure) {
                        settings.iso?.setAutoModeWithoutNotifying(newValue)
                        settings.exposureTimeNS?.setAutoModeWithoutNotifying(newValue)
                    }
                    prop.isInAutoMode = newValue
                    this.autoModeToggle.isSelected = prop.isInAutoMode
                    slider?.handleValueAutoUpdated()
                    handleSettingsAutoChanged()
                }
            }
        }
    }

    private fun initProperties() {
        settingsList.removeAllViews()

        val settings = viewModel.activeCamera.cameraSettings

        val layoutParams = LinearLayout.LayoutParams(
            LinearLayout.LayoutParams.WRAP_CONTENT,
            LinearLayout.LayoutParams.MATCH_PARENT
        )

        settings.properties.forEach { property ->
            if (property !is ManualFocusProperty) {
                val propView: IProperty = when (property) {
                    is BaseToggleProperty -> PropertyToggle(context, property)
                    is BaseRangeProperty -> PropertyRange(context, property)
                    else -> PropertyOptions(context, property as BaseOptionsProperty)
                }
                val view = propView as View
                view.layoutParams = layoutParams
                settingsList.addView(view)
            }
        }

        settingsList.forEach { it
            val propObj = it as IProperty
            val propView = it as View
            val prop = propObj.property

            propView.setOnClickListener {
                if (selectedProperty == propObj) {
                    closeAdditionalControls()
                } else {
                    closeAdditionalControls()
                    when (prop) {
                        is BaseToggleProperty<*> -> prop.toggle()
                        is BaseRangeProperty<*> -> {
                            openRangeContainer(prop)
                            selectedProperty = propObj
                            propView.isSelected = true
                        }
                        is BaseOptionsProperty -> {
                            openOptionListContainer(prop)
                            selectedProperty = propObj
                            propView.isSelected = true
                        }
                    }
                }
            }
        }

        viewModel.activeCamera.onSettingsAutoChanged += ::handleSettingsAutoChanged
    }

    private fun handleSettingsAutoChanged() {
        handler.post {
            settingsList.forEach {
                val propView = it as IProperty
                propView.handleValueChanged()
            }
            if (_isOpen && selectedProperty != null) {
                slider?.let {
                    if (it.property.isInAutoMode) it.handleValueAutoUpdated()
                }
            }
        }
    }

    fun handleCameraSwitched() {
        closeAdditionalControls()
        initProperties()
    }

    private fun openRangeContainer(prop: BaseRangeProperty<*>) {
        _isOpen = true

        val settings = viewModel.activeCamera.cameraSettings

        slider = SliderView(
            context,
            prop,
            {
                selectedProperty?.handleValueChanged()
            }, {
                autoModeToggle.isSelected = false
                if (prop is ManualFocusProperty) {
                    val optProperty = optionsList?.property
                    if (optProperty != null && optProperty is FocusModesProperty) {
                        optProperty.switchToManual()
                        optionsList?.updateManually()
                    }
                } else if ((prop is ISOProperty || prop is ShutterProperty) && !settings.cameraInfo.supportSemiAutoExposure) {
                    settings.iso?.setAutoModeWithoutNotifying(false)
                    settings.exposureTimeNS?.setAutoModeWithoutNotifying(false)
                }
            })

        sliderWrapperContainer.visibility = VISIBLE
        sliderWrapperContainer.addView(slider)

        if (prop.hasSwitcher) {
            autoModeToggle.isVisible = true
            autoModeToggle.isSelected = prop.isInAutoMode
        } else {
            autoModeToggle.isVisible = false
        }
    }
    private fun openOptionListContainer(prop: BaseOptionsProperty) {
        _isOpen = true

        if (prop is FocusModesProperty) {
            optionsList = OptionsList(context, prop) { key ->
                if (key == CameraCharacteristics.CONTROL_AF_MODE_OFF) {
                    viewModel.activeCamera.cameraSettings.manualFocus?.isInAutoMode = false
                } else {
                    viewModel.activeCamera.cameraSettings.manualFocus?.isInAutoMode = true
                }
            }
            previewBottomContainerManager.showGroup(optionsList!!)

            val manualFocus = viewModel.activeCamera.cameraSettings.manualFocus
            if (manualFocus != null) openRangeContainer(manualFocus)
        } else {
            optionsList = OptionsList(context, prop)
            previewBottomContainerManager.showGroup(optionsList!!)
        }
    }
    fun closeAdditionalControls() {
        _isOpen = false
        val selectedView = selectedProperty as? View
        selectedView?.isSelected = false
        selectedProperty = null
        previewBottomContainerManager.clear()
        sliderWrapperContainer.removeAllViews()
        sliderWrapperContainer.visibility = INVISIBLE
        autoModeToggle.isVisible = false
        autoModeToggle.isSelected = false
    }
}