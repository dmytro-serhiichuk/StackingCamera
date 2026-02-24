package com.sedv.stackingcamera.main.settings.ui

import android.annotation.SuppressLint
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.MotionEvent
import android.widget.FrameLayout
import com.sedv.stackingcamera.main.camera.settings.BaseRangeProperty
import com.sedv.stackingcamera.databinding.SliderViewBinding
import kotlin.math.roundToInt

@SuppressLint("Recycle", "ClickableViewAccessibility")
class SliderView(
    context: Context,
    val property: BaseRangeProperty<*>,
    private val onManuallyChangedCallback: () -> Unit,
    private val onAutoModeSwitched: (() -> Unit)? = null
) : FrameLayout(context) {

    private val binding = SliderViewBinding.inflate(LayoutInflater.from(context), this, true)

    private val scrollHandler = Handler(Looper.getMainLooper())
    private var lastScrollX = 0
    private var isSnapping = false
    private val slider: Slider

    init {
        binding.scroll.setOnTouchListener { _, event ->
            if (event.action == MotionEvent.ACTION_DOWN) {
                property.isInAutoMode = false
                onAutoModeSwitched?.invoke()
            }
            if (event.action == MotionEvent.ACTION_UP) {
                waitForScrollIdle()
            }
            false
        }
        binding.scroll.setOnScrollChangeListener { _, newX, newY, oldX, oldY ->
            if (!property.isInAutoMode) {
                val centerX = newX + binding.scroll.width / 2
                val relativeX = centerX - slider.sidePadding
                val index = (relativeX / slider.stepSpacing).roundToInt().coerceIn(0, slider.steps - 1)

                @Suppress("UNCHECKED_CAST")
                property as BaseRangeProperty<Number>
                property.setValueWithNotifying(property.range.mainSteps[index])
                onManuallyChangedCallback()
                binding.valueLabel.text = property.getDisplayValue()
            }
        }

        slider = Slider(context, property.range)
        slider.layoutParams = LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.MATCH_PARENT)
        binding.scroll.addView(slider)
        binding.valueLabel.text = property.getDisplayValue()
    }

    override fun onLayout(changed: Boolean, left: Int, top: Int, right: Int, bottom: Int) {
        super.onLayout(changed, left, top, right, bottom)
        fastScrollToValue(property.value)
    }

    private fun waitForScrollIdle() {
        if (isSnapping) return

        isSnapping = true
        lastScrollX = binding.scroll.scrollX

        scrollHandler.postDelayed(object : Runnable {
            override fun run() {
                val currentScrollX = binding.scroll.scrollX
                if (currentScrollX == lastScrollX) {
                    val centerX = currentScrollX + binding.scroll.width / 2
                    val relativeX = centerX - slider.sidePadding
                    val index = (relativeX / slider.stepSpacing).roundToInt().coerceIn(0, slider.steps - 1)
                    val targetX = (index * slider.stepSpacing + slider.sidePadding - binding.scroll.width / 2).toInt()
                    binding.scroll.smoothScrollTo(targetX, 0)
                    isSnapping = false

                    @Suppress("UNCHECKED_CAST")
                    property as BaseRangeProperty<Number>
                    property.setValueWithNotifying(property.range.mainSteps[index])
                    binding.valueLabel.text = property.getDisplayValue()
                } else {
                    lastScrollX = currentScrollX
                    scrollHandler.postDelayed(this, 50)
                }
            }
        }, 100)
    }

    fun handleValueAutoUpdated() {
        if (!isSnapping) {
            binding.valueLabel.text = property.getDisplayValue()
            smoothScrollToValue(property.value)
        }
    }

    private fun smoothScrollToValue(target: Number) {
        @Suppress("UNCHECKED_CAST")
        property as BaseRangeProperty<Number>
        val index = property.getRangeIndexFromValue(target)
        val targetX = (index * slider.stepSpacing + slider.sidePadding - binding.scroll.width / 2).toInt()
        binding.scroll.smoothScrollTo(targetX, 0)
    }
    private fun fastScrollToValue(target: Number) {
        @Suppress("UNCHECKED_CAST")
        property as BaseRangeProperty<Number>
        val index = property.getRangeIndexFromValue(target)
        val targetX = (index * slider.stepSpacing + slider.sidePadding - binding.scroll.width / 2).toInt()
        binding.scroll.scrollTo(targetX, 0)
    }
}