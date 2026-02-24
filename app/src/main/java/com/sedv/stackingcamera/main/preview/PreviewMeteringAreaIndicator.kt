package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.os.Handler
import android.os.Looper
import android.util.AttributeSet
import android.view.View
import com.sedv.stackingcamera.R

class PreviewMeteringAreaIndicator(
    context: Context,
    attrs: AttributeSet? = null
) : View(context, attrs) {

    private var focusRect: RectF? = null
    private val paint = Paint().apply {
        style = Paint.Style.STROKE
        color = Color.WHITE
        strokeWidth = context.resources.getDimension(R.dimen.metering_area_stroke_width)
        isAntiAlias = true
    }

    private val durationMs = 5000L
    private val handler = Handler(Looper.getMainLooper())
    private var hideRunnable: Runnable? = null

    private var desiredWidth: Int = 0
    private var desiredHeight: Int = 0

    fun showFocusAt(x: Float, y: Float, size: Float) {
        focusRect = RectF(
            x - size / 2,
            y - size / 2,
            x + size / 2,
            y + size / 2
        )
        invalidate()

        hideRunnable = Runnable { hide() }

        hideRunnable?.let {
            handler.postDelayed(it, durationMs)
        }

    }

    fun hide() {
        focusRect = null
        invalidate()

        hideRunnable?.let {
            handler.removeCallbacks(it)
            hideRunnable = null
        }
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        focusRect?.let {
            canvas.drawRect(it, paint)
        }
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)
        if (desiredWidth == 0 || desiredHeight == 0) {
            setMeasuredDimension(width, height)
        } else {
            setMeasuredDimension(desiredWidth, desiredHeight)
        }
    }

    fun setSize(width: Int, height: Int) {
        desiredWidth = width
        desiredHeight = height
        requestLayout()
        invalidate()
    }
}