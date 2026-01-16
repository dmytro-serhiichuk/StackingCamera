package com.sedv.stackingcamera.stacking.settings

enum class ImageFormatSupport(val value: Int) {
    NONE(0),
    UINT8(1),
    UINT16(2),
    FULL(3);

    companion object {
        fun fromInt(value: Int) = entries.first { it.value == value }
    }
}

data class AvailableImageSettings(
    val IMAGE_RGB_SUPPORT: ImageFormatSupport,
    val IMAGE_RGBA_SUPPORT: ImageFormatSupport,
) {
    constructor(rgbValue: Int, rgbaValue: Int) : this(
        ImageFormatSupport.fromInt(rgbValue),
        ImageFormatSupport.fromInt(rgbaValue)
    )
}
