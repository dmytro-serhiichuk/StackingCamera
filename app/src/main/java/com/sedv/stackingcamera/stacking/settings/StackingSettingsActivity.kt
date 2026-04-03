package com.sedv.stackingcamera.stacking.settings

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.sedv.stackingcamera.databinding.ActivityStackingSettingsBinding
import com.sedv.stackingcamera.stacking.StackingHandler
import kotlin.math.roundToInt

class StackingSettingsActivity : AppCompatActivity() {
    private lateinit var binding: ActivityStackingSettingsBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityStackingSettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.navBackButton.setOnClickListener {
            finish()
        }

        binding.root.post {
            binding.use16BitData.init(Settings.USE_16_BIT.value)
            binding.colorSpace.init(
                Settings.COLOR_SPACE.options.map { ColorSpace.fromInt(it).nameValue },
                ColorSpace.fromInt(Settings.COLOR_SPACE.value).nameValue
            )
            binding.numberOfTiles.init(
                Settings.TILES_PER_SIDE.range.lower.toFloat(),
                Settings.TILES_PER_SIDE.range.upper.toFloat(),
                Settings.TILES_PER_SIDE.value.toFloat(),
                Settings.TILES_PER_SIDE.step.toFloat(),
                { value ->
                    val rounded = value.roundToInt()
                    "${rounded * rounded}"
                }
            )

            binding.maxKeypoints.init(
                Settings.MAX_KEYPOINTS.range.lower.toFloat(),
                Settings.MAX_KEYPOINTS.range.upper.toFloat(),
                Settings.MAX_KEYPOINTS.value.toFloat(),
                Settings.MAX_KEYPOINTS.step.toFloat(),
                { value ->
                    value.roundToInt().toString()
                }
            )
            binding.fastThreshold.init(
                Settings.FAST_THRESHOLD.range.lower.toFloat(),
                Settings.FAST_THRESHOLD.range.upper.toFloat(),
                Settings.FAST_THRESHOLD.value.toFloat(),
                Settings.FAST_THRESHOLD.step.toFloat(),
                { value ->
                    value.roundToInt().toString()
                }
            )
            binding.mBriskPatternScaleFactor.init(
                Settings.BRISK_PATTERNS_SCALE.range.lower,
                Settings.BRISK_PATTERNS_SCALE.range.upper,
                Settings.BRISK_PATTERNS_SCALE.value,
                Settings.BRISK_PATTERNS_SCALE.step,
                { value ->
                    value.toString()
                }
            )
            binding.saveKeypoints.init(Settings.SAVE_KEYPOINTS.value)

            binding.maxMatches.init(
                Settings.MAX_MATCHES.range.lower.toFloat(),
                Settings.MAX_MATCHES.range.upper.toFloat(),
                Settings.MAX_MATCHES.value.toFloat(),
                Settings.MAX_MATCHES.step.toFloat(),
                { value ->
                    value.roundToInt().toString()
                }
            )
            binding.saveMatches.init(Settings.SAVE_MATCHES.value)

            binding.ransacThreshold.init(
                Settings.RANSAC_THRESHOLD.range.lower,
                Settings.RANSAC_THRESHOLD.range.upper,
                Settings.RANSAC_THRESHOLD.value,
                Settings.RANSAC_THRESHOLD.step,
                { value ->
                    value.toString()
                }
            )
            binding.ransacIterations.init(
                Settings.RANSAC_ITERATIONS.range.lower.toFloat(),
                Settings.RANSAC_ITERATIONS.range.upper.toFloat(),
                Settings.RANSAC_ITERATIONS.value.toFloat(),
                Settings.RANSAC_ITERATIONS.step.toFloat(),
                { value ->
                    value.roundToInt().toString()
                }
            )
        }

        binding.restoreButton.setOnClickListener { handleRestoreButtonClicked() }
        binding.saveButton.setOnClickListener { handleSaveButtonClicked() }

        // TODO add tooltip handling
    }

    private fun handleRestoreButtonClicked() {
        Settings.resetSettings()

        binding.use16BitData.isChecked = Settings.USE_16_BIT.value
        binding.colorSpace.value = ColorSpace.fromInt(Settings.COLOR_SPACE.value).nameValue
        binding.numberOfTiles.value = Settings.TILES_PER_SIDE.value.toFloat()

        binding.maxKeypoints.value = Settings.MAX_KEYPOINTS.value.toFloat()
        binding.fastThreshold.value = Settings.FAST_THRESHOLD.value.toFloat()
        binding.mBriskPatternScaleFactor.value = Settings.BRISK_PATTERNS_SCALE.value
        binding.saveKeypoints.isChecked = Settings.SAVE_KEYPOINTS.value

        binding.maxMatches.value = Settings.MAX_MATCHES.value.toFloat()
        binding.saveMatches.isChecked = Settings.SAVE_MATCHES.value

        binding.ransacThreshold.value = Settings.RANSAC_THRESHOLD.value
        binding.ransacIterations.value = Settings.RANSAC_ITERATIONS.value.toFloat()

        applyNewValues()
    }

    private fun handleSaveButtonClicked() {
        Settings.USE_16_BIT.value = binding.use16BitData.isChecked
        Settings.COLOR_SPACE.value = ColorSpace.fromString(binding.colorSpace.value).value
        Settings.TILES_PER_SIDE.value = binding.numberOfTiles.value.toInt()

        Settings.MAX_KEYPOINTS.value = binding.maxKeypoints.value.toInt()
        Settings.FAST_THRESHOLD.value = binding.fastThreshold.value.toInt()
        Settings.BRISK_PATTERNS_SCALE.value = binding.mBriskPatternScaleFactor.value
        Settings.SAVE_KEYPOINTS.value = binding.saveKeypoints.isChecked

        Settings.MAX_MATCHES.value = binding.maxMatches.value.toInt()
        Settings.SAVE_MATCHES.value = binding.saveMatches.isChecked

        Settings.RANSAC_THRESHOLD.value = binding.ransacThreshold.value
        Settings.RANSAC_ITERATIONS.value = binding.ransacIterations.value.toInt()

        applyNewValues()
        Settings.updateSettings()
    }

    private fun applyNewValues() {
        if (binding.numberOfTiles.isChanged || binding.use16BitData.isChanged || binding.colorSpace.isChanged) {
            StackingHandler.hasMatches = false
            StackingHandler.removeAnalyseResults()
        }

        binding.use16BitData.applyNewValue()
        binding.colorSpace.applyNewValue()
        binding.numberOfTiles.applyNewValue()
        binding.maxKeypoints.applyNewValue()
        binding.fastThreshold.applyNewValue()
        binding.mBriskPatternScaleFactor.applyNewValue()
        binding.saveKeypoints.applyNewValue()
        binding.maxMatches.applyNewValue()
        binding.saveMatches.applyNewValue()
        binding.ransacThreshold.applyNewValue()
        binding.ransacIterations.applyNewValue()
    }
}