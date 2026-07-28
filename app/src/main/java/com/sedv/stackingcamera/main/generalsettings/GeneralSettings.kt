package com.sedv.stackingcamera.main.generalsettings

import android.graphics.ImageFormat
import com.sedv.stackingcamera.Event
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.main.camera.Camera
import com.sedv.stackingcamera.main.generalsettings.property.BaseProperty
import com.sedv.stackingcamera.main.generalsettings.property.OptionProperty
import com.sedv.stackingcamera.main.generalsettings.property.ToggleProperty

object GeneralSettings {
    val properties: Set<BaseProperty<*>>
    val onChanged = Event<(BaseProperty<*>) -> Unit>()

    val histogram: ToggleProperty
    val frameSize: OptionProperty
    val timer: OptionProperty
    val focusPeaking: ToggleProperty
    val ghostImage: ToggleProperty

    init {
        properties = LinkedHashSet()

        histogram = ToggleProperty(
            GeneralPropertyType.HISTOGRAM,
            ::onPropertyChanged,
            false,
            R.drawable.icons_histogram_disabled
        )
        properties.add(histogram)

        properties.add(
            ToggleProperty(
                GeneralPropertyType.GRID,
                ::onPropertyChanged,
                false,
                R.drawable.icons_grid_disabled
            )
        )
        timer = OptionProperty(
            GeneralPropertyType.TIMER,
            ::onPropertyChanged,
            0,
            mapOf<Int, Int>(
                0 to R.drawable.icon_timer_0,
                2 to R.drawable.icon_timer_2,
                5 to R.drawable.icon_timer_5,
                10 to R.drawable.icon_timer_10
            )
        )
        properties.add(timer)
        focusPeaking = ToggleProperty(
            GeneralPropertyType.FOCUS_PEAKING,
            ::onPropertyChanged,
            false,
            R.drawable.icons_focus_peaking_disabled
        )
        properties.add(focusPeaking)
        properties.add(
            ToggleProperty(
                GeneralPropertyType.ZEBRA_PATTERN,
                ::onPropertyChanged,
                false,
                R.drawable.icons_zebra_disabled
            )
        )

        frameSize = OptionProperty(
            GeneralPropertyType.FRAME_SIZE,
            ::onPropertyChanged,
            FrameSize.FRAME_SIZE_4_3.value,
            mapOf<Int, Int>(
                FrameSize.FRAME_SIZE_4_3.value to R.drawable.icons_frame_size_4_3,
                FrameSize.FRAME_SIZE_16_9.value to R.drawable.icons_frame_size_16_9
            )
        )
        properties.add(frameSize)

         ghostImage = ToggleProperty(
            GeneralPropertyType.GHOST_IMAGE,
            ::onPropertyChanged,
            false,
            R.drawable.icons_ghost_disabled
        )
        properties.add(ghostImage)
    }

    fun handleCameraSwitched(activeCamera: Camera) {
        histogram.isAvailable = activeCamera.cameraInfo.tryGetSupportedFormat(ImageFormat.YUV_420_888) != null
        focusPeaking.isAvailable = activeCamera.cameraSettings.focusModes != null
    }

    private fun onPropertyChanged(prop: BaseProperty<*>) {
        onChanged.invokeAll { it.invoke(prop) }
    }
}

enum class GeneralPropertyType(val drawable: Int) {
    HISTOGRAM(R.drawable.icons_histogram),
    GRID(R.drawable.icons_grid),
    TIMER(R.drawable.icon_timer_0),
    FOCUS_PEAKING(R.drawable.icons_focus_peaking),
    ZEBRA_PATTERN(R.drawable.icons_zebra),
    FRAME_SIZE(R.drawable.icons_frame_size_4_3),
    GHOST_IMAGE(R.drawable.icons_ghost)
}

enum class FrameSize(val value: Int) {
    FRAME_SIZE_4_3(0),
    FRAME_SIZE_16_9(1),
}