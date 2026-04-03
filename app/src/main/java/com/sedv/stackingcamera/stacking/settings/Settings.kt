package com.sedv.stackingcamera.stacking.settings

import android.content.SharedPreferences
import android.util.Range
import kotlin.collections.arrayListOf

enum class ColorSpace(val value: Int, val nameValue: String) {
    sRGB(0, "sRGB"),
    Linear_sRGB(1, "Linear sRGB"),
    AdobeRGB(2, "Adobe RGB"),
    WideGamut(3, "WideGamut"),
    ProPhoto(4, "ProPhoto");

    companion object {
        fun fromInt(value: Int) = ColorSpace.entries.first { it.value == value }
        fun fromString(value: String) = ColorSpace.entries.first { it.nameValue == value }
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
    val COLOR_SPACE: OptionsProperty
    val SAVE_KEYPOINTS = BoolProperty("SAVE_KEYPOINTS", false)
    val SAVE_MATCHES = BoolProperty("SAVE_MATCHES", false)

    val properties: ArrayList<Property<*>>

    private lateinit var sharedPreferences: SharedPreferences

    init {
        System.loadLibrary("stackingcamera")

        COLOR_SPACE = OptionsProperty(
            "COLOR_SPACE",
            ColorSpace.sRGB.value,
            linkedSetOf(
                ColorSpace.sRGB.value,
                ColorSpace.Linear_sRGB.value,
                ColorSpace.AdobeRGB.value,
                ColorSpace.WideGamut.value,
                ColorSpace.ProPhoto.value
            )
        )

        properties = arrayListOf(
            FAST_THRESHOLD,
            RANSAC_THRESHOLD,
            RANSAC_ITERATIONS,
            TILES_PER_SIDE,
            MAX_KEYPOINTS,
            MAX_MATCHES,
            BRISK_PATTERNS_SCALE,
            USE_16_BIT,
            COLOR_SPACE,
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

    private fun callApplySettings() {
        applySettings(
            FAST_THRESHOLD.value,
            RANSAC_THRESHOLD.value,
            RANSAC_ITERATIONS.value,
            TILES_PER_SIDE.value,
            MAX_KEYPOINTS.value,
            MAX_MATCHES.value,
            BRISK_PATTERNS_SCALE.value,
            USE_16_BIT.value,
            COLOR_SPACE.value,
            SAVE_KEYPOINTS.value,
            SAVE_MATCHES.value
        )
    }

    // Native functions
    external private fun applySettings(
        fastThreshold: Int,
        ransacThreshold: Float,
        ransacIterations: Int,
        tilesPerSide: Int,
        maxKeypoints: Int,
        maxMatches: Int,
        briskPatternScale: Float,
        use16Bit: Boolean,
        colorSpace: Int,
        saveKeypoints: Boolean,
        saveMatches: Boolean
    )
}