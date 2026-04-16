package com.sedv.stackingcamera.stacking

import android.animation.ValueAnimator
import android.content.Context
import android.util.AttributeSet
import android.view.LayoutInflater
import android.widget.LinearLayout
import androidx.core.animation.doOnEnd
import androidx.core.content.ContextCompat
import androidx.core.view.isVisible
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.databinding.BitmapWarningViewBinding

enum class WarningLevel { WARNING, BAD }

class BitmapWarningView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : LinearLayout(context, attrs) {

    private val binding = BitmapWarningViewBinding.inflate(LayoutInflater.from(context), this, true)

    private var isExpanded = false

    private var bg: Int = 0
    private var border: Int = 0
    private var text: Int = 0

    init {
        binding.chipButton.setOnClickListener {
            isExpanded = !isExpanded
            binding.chipChevron.text = if (isExpanded) "▲" else "▼"

            if (isExpanded) {
                expandDetails()
            } else {
                collapseDetails()
            }
        }
    }

    fun setValues(level: WarningLevel, message: String) {
        isExpanded = false

        val styles = when (level) {
            WarningLevel.WARNING -> WarningStyle(
                R.color.warning_bg,
                R.color.warning_border,
                R.color.warning_text
            )
            WarningLevel.BAD -> WarningStyle(
                R.color.error_bg,
                R.color.error_border,
                R.color.error_text
            )
        }

        bg = ContextCompat.getColor(context, styles.bgRes)
        border = ContextCompat.getColor(context, styles.borderRes)
        text = ContextCompat.getColor(context, styles.textRes)

        binding.chipButton.setBackgroundColor(bg)
        binding.chipButton.background = buildRoundedBackground(
            bg, border,
            true, !isExpanded
        )

        binding.chipLabel.setTextColor(text)
        binding.chipChevron.setTextColor(text)

        binding.detailsText.text = message
        binding.detailsText.setTextColor(text)
        binding.detailsContainer.isVisible = false
        binding.detailsContainer.background = buildRoundedBackground(
            bg, border,
            false, true
        )
    }

    private fun expandDetails() {
        binding.detailsContainer.isVisible = true
        binding.detailsContainer.measure(
            MeasureSpec.makeMeasureSpec(width, MeasureSpec.EXACTLY),
            MeasureSpec.makeMeasureSpec(0, MeasureSpec.UNSPECIFIED)
        )
        val targetHeight = binding.detailsContainer.measuredHeight

        binding.detailsContainer.layoutParams.height = 0
        binding.detailsContainer.requestLayout()

        binding.chipButton.background = buildRoundedBackground(
            bg, border,
            true, !isExpanded
        )

        ValueAnimator.ofInt(0, targetHeight).apply {
            duration = 220
            addUpdateListener { animator ->
                binding.detailsContainer.layoutParams.height = animator.animatedValue as Int
                binding.detailsContainer.requestLayout()
            }
            start()
        }
    }

    private fun collapseDetails() {
        val initialHeight = binding.detailsContainer.measuredHeight

        ValueAnimator.ofInt(initialHeight, 0).apply {
            duration = 180
            addUpdateListener { animator ->
                val h = animator.animatedValue as Int
                if (h == 0) {
                    binding.detailsContainer.isVisible = false
                } else {
                    binding.detailsContainer.layoutParams.height = h
                    binding.detailsContainer.requestLayout()
                }
            }
            doOnEnd {
                binding.chipButton.background = buildRoundedBackground(
                    bg, border,
                    true, !isExpanded
                )
            }
            start()
        }
    }

    private fun buildRoundedBackground(
        fillColor: Int,
        strokeColor: Int,
        topRounded: Boolean,
        bottomRounded: Boolean
    ): android.graphics.drawable.GradientDrawable {
        val dp = resources.displayMetrics.density
        val radius = 8 * dp

        return android.graphics.drawable.GradientDrawable().apply {
            shape = android.graphics.drawable.GradientDrawable.RECTANGLE
            setColor(fillColor)
            setStroke((1.5 * dp).toInt(), strokeColor)
            val topRadius = if (topRounded) radius else 0f
            val bottomRadius = if (bottomRounded) radius else 0f
            cornerRadii = floatArrayOf(
                topRadius, topRadius,   // top-left
                topRadius, topRadius,   // top-right
                bottomRadius, bottomRadius, // bottom-right
                bottomRadius, bottomRadius  // bottom-left
            )
        }
    }

    private data class WarningStyle(
        val bgRes: Int,
        val borderRes: Int,
        val textRes: Int,
    )
}