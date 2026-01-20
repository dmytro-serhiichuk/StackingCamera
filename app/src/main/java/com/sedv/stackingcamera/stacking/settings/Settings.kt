package com.sedv.stackingcamera.stacking.settings

import android.content.SharedPreferences
import android.util.Range
import kotlin.collections.arrayListOf

enum class ImageFormat(val value: Int, val nameValue: String) {
    NONE(0, "Not Supported"),
    RGB_8(1, "8-Bit RGB"),
    RGB_16(2, "16-Bit RGB"),
    RGBA_8(3, "8-Bit RGBA"),
    RGBA_16(4, "16-Bit RGBA");

    companion object {
        fun fromInt(value: Int) = ImageFormat.entries.first { it.value == value }
        fun fromString(value: String) = ImageFormat.entries.first { it.nameValue == value }
    }
}

object Settings {
    val FAST_THRESHOLD = RangedProperty("FAST_THRESHOLD", 20, Range(5, 60), 1)
    val RANSAC_THRESHOLD = RangedProperty("RANSAC_THRESHOLD", 1.0f, Range(0.5f, 10.0f), 0.5f)
    val RANSAC_ITERATIONS = RangedProperty("RANSAC_ITERATIONS", 10_000, Range(1_000, 30_000), 250)
    val TILES_PER_SIDE = RangedProperty("TILES_PER_SIDE", 6, Range(1, 10), 1)
    val MAX_KEYPOINTS = RangedProperty("MAX_KEYPOINTS", 20_000, Range(5_000, 100_000), 100)
    val MAX_MATCHES = RangedProperty("MAX_MATCHES", 500, Range(50, 2000), 10)
    val BRISK_PATTERNS_SCALE = RangedProperty("BRISK_PATTERNS_SCALE", 10.0f, Range(1.0f, 20.0f), 0.5f)
    val USE_16_BIT = BoolProperty("USE_16_BIT", true)
    val USE_IMAGES = BoolProperty("USE_IMAGES", false)
    val IMAGES_FORMAT: OptionsProperty
    val SAVE_KEYPOINTS = BoolProperty("SAVE_KEYPOINTS", false)
    val SAVE_MATCHES = BoolProperty("SAVE_MATCHES", false)

    val properties: ArrayList<Property<*>>

    private val availableImageSettings: AvailableImageSettings
    private lateinit var sharedPreferences: SharedPreferences

    init {
        System.loadLibrary("stackingcamera")

        availableImageSettings = getAvailableImageSettings()
        val imageFormats = createImageFormatOptions()

        IMAGES_FORMAT = OptionsProperty("IMAGES_FORMAT", imageFormats.toList()[0], imageFormats)

        properties = arrayListOf(
            FAST_THRESHOLD,
            RANSAC_THRESHOLD,
            RANSAC_ITERATIONS,
            TILES_PER_SIDE,
            MAX_KEYPOINTS,
            MAX_MATCHES,
            BRISK_PATTERNS_SCALE,
            USE_16_BIT,
            USE_IMAGES,
            IMAGES_FORMAT,
            SAVE_KEYPOINTS,
            SAVE_MATCHES
        )
    }

    fun initialize(sp: SharedPreferences) {
        sharedPreferences = sp

        for (prop in properties) {
            prop.loadFrom(sharedPreferences)
        }

        callApplySettings()
    }
    private fun createImageFormatOptions(): LinkedHashSet<Int> {
        val options = linkedSetOf<Int>()
        if (availableImageSettings.IMAGE_RGB_SUPPORT == ImageFormatSupport.FULL) {
            options.add(ImageFormat.RGB_8.value)
            options.add(ImageFormat.RGB_16.value)
        } else if (availableImageSettings.IMAGE_RGB_SUPPORT == ImageFormatSupport.UINT8) {
            options.add(ImageFormat.RGB_8.value)
        } else if (availableImageSettings.IMAGE_RGB_SUPPORT == ImageFormatSupport.UINT16) {
            options.add(ImageFormat.RGB_16.value)
        }

        if (availableImageSettings.IMAGE_RGBA_SUPPORT == ImageFormatSupport.FULL) {
            options.add(ImageFormat.RGBA_8.value)
            options.add(ImageFormat.RGBA_16.value)
        } else if (availableImageSettings.IMAGE_RGBA_SUPPORT == ImageFormatSupport.UINT8) {
            options.add(ImageFormat.RGBA_8.value)
        } else if (availableImageSettings.IMAGE_RGBA_SUPPORT == ImageFormatSupport.UINT16) {
            options.add(ImageFormat.RGBA_16.value)
        }

        if (options.isEmpty()) {
            options.add(ImageFormat.NONE.value)
        }

        return options
    }

    fun updateSettings() {
        val editor = sharedPreferences.edit()

        for (prop in properties) {
            prop.saveTo(editor)
        }

        editor.commit()
        callApplySettings()
    }

    fun resetSettings() {
        val editor = sharedPreferences.edit()

        for (prop in properties) {
            prop.reset(editor)
        }

        editor.commit()
        callApplySettings()
    }

    inline private fun callApplySettings() {
        applySettings(
            FAST_THRESHOLD.value,
            RANSAC_THRESHOLD.value,
            RANSAC_ITERATIONS.value,
            TILES_PER_SIDE.value,
            MAX_KEYPOINTS.value,
            MAX_MATCHES.value,
            BRISK_PATTERNS_SCALE.value,
            USE_16_BIT.value,
            USE_IMAGES.value,
            IMAGES_FORMAT.value,
            SAVE_KEYPOINTS.value,
            SAVE_MATCHES.value
        )
    }

    // Native functions
    external private fun getAvailableImageSettings(): AvailableImageSettings
    external private fun applySettings(
        fastThreshold: Int,
        ransacThreshold: Float,
        ransacIterations: Int,
        tilesPerSide: Int,
        maxKeypoints: Int,
        maxMatches: Int,
        briskPatternScale: Float,
        use16Bit: Boolean,
        useImages: Boolean,
        imageFormat: Int,
        saveKeypoints: Boolean,
        saveMatches: Boolean
    )
}