package com.sedv.stackingcamera.stacking

import android.content.Context
import android.graphics.Color
import android.text.SpannableStringBuilder
import android.text.Spanned
import android.text.style.ForegroundColorSpan
import android.util.AttributeSet
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import com.google.android.material.card.MaterialCardView
import com.sedv.stackingcamera.databinding.LogWindowBinding
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

class LogWindow @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : MaterialCardView(context, attrs, defStyleAttr) {

    private val binding = LogWindowBinding.inflate(LayoutInflater.from(context), this, true)
    private var processingJob: Job? = null

    var onCloseButtonClicked: (() -> Unit)? = null

    private var isScrollAtBottom = true

    init {
        binding.closeButton.setOnClickListener {
            onCloseButtonClicked?.invoke()
        }

        binding.scrollContainer.viewTreeObserver.addOnScrollChangedListener {
            val diff = binding.textContainer.bottom - (binding.scrollContainer.height + binding.scrollContainer.scrollY)
            isScrollAtBottom = diff <= STICKY_SCROLL_MAX_DIFFERENCE
        }
    }

    fun addMessage(text: String, isError: Boolean = false) {
        val builder = when (val current = binding.textContainer.text) {
            is SpannableStringBuilder -> current
            else -> SpannableStringBuilder(current)
        }

        val start = builder.length
        builder.append(text).append('\n')
        val end = builder.length

        if (isError) {
            builder.setSpan(
                ForegroundColorSpan(Color.RED),
                start,
                end,
                Spanned.SPAN_EXCLUSIVE_EXCLUSIVE
            )
        }

        binding.textContainer.text = builder
        if (isScrollAtBottom) {
            binding.scrollContainer.post {
                binding.scrollContainer.fullScroll(View.FOCUS_DOWN)
            }
        }
    }

    fun reset() {
        binding.textContainer.text = ""
        binding.closeButton.isEnabled = false

        processingJob?.cancel()
        processingJob = CoroutineScope(Dispatchers.Main).launch {
            val baseText = "Processing"
            var dots = 0

            while (true) {
                val suffix = ".".repeat(dots)
                binding.headerText.text = baseText + suffix

                dots = (dots + 1) % 4
                delay(500)
            }
        }
    }

    fun markFinished() {
        binding.headerText.text = "Completed"
        binding.closeButton.isEnabled = true
        processingJob?.cancel()
        processingJob = null
    }

    companion object {
        private const val STICKY_SCROLL_MAX_DIFFERENCE = 100
    }
}