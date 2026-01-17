package com.sedv.stackingcamera.stacking.settings

import android.content.SharedPreferences
import android.util.Range
import kotlin.collections.arrayListOf

object Settings {
    val FAST_THRESHOLD = RangedProperty("FAST_THRESHOLD", 20, Range(5, 60))
    val RANSAC_THRESHOLD = RangedProperty("RANSAC_THRESHOLD", 1.0f, Range(0.5f, 5.0f))
    val RANSAC_ITERATIONS = RangedProperty("RANSAC_ITERATIONS", 10_000, Range(1_000, 30_000))
    val CHUNKS_PER_SIDE = RangedProperty("CHUNKS_PER_SIDE", 6, Range(1, 10))
    val MAX_KEYPOINTS_COUNT = RangedProperty("MAX_KEYPOINTS_COUNT", 20_000, Range(5_000, 100_000))
    val MAX_MATCHES_COUNT = RangedProperty("MAX_MATCHES_COUNT", 500, Range(50, 2000))
    val BRISK_PATTERNS_SCALE = RangedProperty("BRISK_PATTERNS_SCALE", 10.0f, Range(1.0f, 15.0f))
    val USE_16_BIT_BITMAPS = BoolProperty("USE_16_BIT_BITMAPS", true)
    val USE_IMAGES = BoolProperty("USE_IMAGES", false)
    val USE_RGB_IMAGES = BoolProperty("USE_RGB_IMAGES", false)
    val USE_16_BIT_IMAGES = BoolProperty("USE_16_BIT_IMAGES", false)
    val DRAW_KEYPOINTS = BoolProperty("DRAW_KEYPOINTS", false)
    val DRAW_MATCHES = BoolProperty("DRAW_MATCHES", false)

    val properties: ArrayList<Property<*>>

    private lateinit var availableImageSettings: AvailableImageSettings
    private lateinit var sharedPreferences: SharedPreferences

    init {
        System.loadLibrary("stackingcamera")

        properties = arrayListOf(
            FAST_THRESHOLD,
            RANSAC_THRESHOLD,
            RANSAC_ITERATIONS,
            CHUNKS_PER_SIDE,
            MAX_KEYPOINTS_COUNT,
            MAX_MATCHES_COUNT,
            BRISK_PATTERNS_SCALE,
            USE_16_BIT_BITMAPS,
            USE_IMAGES,
            USE_RGB_IMAGES,
            USE_16_BIT_IMAGES,
            DRAW_KEYPOINTS,
            DRAW_MATCHES
        )
    }

    fun initialize(sp: SharedPreferences) {
        sharedPreferences = sp

        availableImageSettings = getAvailableImageSettings()

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

    inline fun callApplySettings() {
        applySettings(
            FAST_THRESHOLD.value,
            RANSAC_THRESHOLD.value,
            RANSAC_ITERATIONS.value,
            CHUNKS_PER_SIDE.value,
            MAX_KEYPOINTS_COUNT.value,
            MAX_MATCHES_COUNT.value,
            BRISK_PATTERNS_SCALE.value,
            USE_16_BIT_BITMAPS.value,
            USE_IMAGES.value,
            USE_RGB_IMAGES.value,
            USE_16_BIT_IMAGES.value,
            DRAW_KEYPOINTS.value,
            DRAW_MATCHES.value
        )
    }

    // Native functions
    external fun getAvailableImageSettings(): AvailableImageSettings
    external fun applySettings(
        fastThreshold: Int,
        ransacThreshold: Float,
        ransacIterations: Int,
        chunksPerSide: Int,
        maxKeypoints: Int,
        maxMatches: Int,
        briskPatternScale: Float,
        use16BitBitmaps: Boolean,
        useImages: Boolean,
        useRgbImages: Boolean,
        use16BitImages: Boolean,
        drawKeypoints: Boolean,
        drawMatches: Boolean
    )
}