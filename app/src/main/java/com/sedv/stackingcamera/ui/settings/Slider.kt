package com.sedv.stackingcamera.ui.settings

import android.content.Context
import android.content.res.Resources
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.view.View
import android.widget.Toast
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.camera.settings.Range

class Slider(
    context: Context,
    val range: Range<*>
) : View(context) {

    var stepSpacing = context.resources.getDimension(R.dimen.slider_step_spacing)
    val steps = range.mainSteps.size

    private val tickPaint = Paint().apply {
        color = Color.GRAY
        strokeWidth = context.resources.getDimension(R.dimen.slider_minor_tick_width)
    }

    private val majorTickPaint = Paint().apply {
        color = Color.WHITE
        strokeWidth = context.resources.getDimension(R.dimen.slider_major_tick_width)
    }

    val sidePadding: Float
        get() {
            val screenWidth = Resources.getSystem().displayMetrics.widthPixels
            return screenWidth / 2f
        }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val contentWidth = ((steps - 1) * stepSpacing).toInt()
        val totalWidth = (contentWidth + 2 * sidePadding).toInt()
        val height = MeasureSpec.getSize(heightMeasureSpec)

        setMeasuredDimension(totalWidth, height)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        if (range.mainSteps.isEmpty()) return

        var nextKeyStepIndex = 0
        for (i in 0 until steps) {
            val x = sidePadding + i * stepSpacing

            val isMajor = range.mainSteps[i] == range.keySteps[nextKeyStepIndex]
            val yStart = if (isMajor) {
                nextKeyStepIndex++
                height * 0.7f
            } else {
                height * 0.8f
            }

            val yEnd = height.toFloat()

            canvas.drawLine(x, yStart, x, yEnd, if (isMajor) majorTickPaint else tickPaint)
        }
    }
}