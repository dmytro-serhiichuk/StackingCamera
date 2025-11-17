package com.sedv.stackingcamera.settings

import com.sedv.stackingcamera.Event
import com.sedv.stackingcamera.R

object GeneralSettings {
    val properties: Set<BaseProperty<*>>
    val onChanged = Event<(BaseProperty<*>) -> Unit>()

    val histogram: ToggleProperty
    val frameSize: OptionProperty
    val timer: OptionProperty

    init {
        properties = LinkedHashSet<BaseProperty<*>>()

        histogram = ToggleProperty(
            GeneralPropertyType.HISTOGRAM,
            ::onPropertyChanged,
            false,
            R.drawable.icons_histogram_disabled
        )
        properties.add(histogram)

        properties.add(ToggleProperty(
            GeneralPropertyType.GRID,
            ::onPropertyChanged,
            false,
            R.drawable.icons_grid_disabled
        ))
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
        properties.add(ToggleProperty(
            GeneralPropertyType.FOCUS_PEAKING,
            ::onPropertyChanged,
            false,
            R.drawable.icons_focus_peaking_disabled
        ))
        properties.add(ToggleProperty(
            GeneralPropertyType.ZEBRA_PATTERN,
            ::onPropertyChanged,
            false,
            R.drawable.icons_zebra_disabled
        ))

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
    FRAME_SIZE(R.drawable.icons_frame_size_4_3)
}

enum class FrameSize(val value: Int) {
    FRAME_SIZE_4_3(0),
    FRAME_SIZE_16_9(1),
}