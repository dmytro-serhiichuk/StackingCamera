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
    private val focusedPaint = Paint().apply {
        style = Paint.Style.STROKE
        color = Color.GREEN
        strokeWidth = context.resources.getDimension(R.dimen.metering_area_stroke_width)
        isAntiAlias = true
    }
    private val notFocusedPaint = Paint().apply {
        style = Paint.Style.STROKE
        color = Color.RED
        strokeWidth = context.resources.getDimension(R.dimen.metering_area_stroke_width)
        isAntiAlias = true
    }

    private var currentPaint = paint

    private var desiredWidth: Int = 0
    private var desiredHeight: Int = 0

    fun showFocusAt(x: Float, y: Float, size: Float) {
        currentPaint = paint

        focusRect = RectF(
            x - size / 2,
            y - size / 2,
            x + size / 2,
            y + size / 2
        )
        invalidate()

    }

    fun hide() {
        focusRect = null
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        focusRect?.let {
            canvas.drawRect(it, currentPaint)
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

    fun handleFocusStateUpdated(state: Boolean) {
        currentPaint = if (state) focusedPaint else notFocusedPaint
        invalidate()
    }
}