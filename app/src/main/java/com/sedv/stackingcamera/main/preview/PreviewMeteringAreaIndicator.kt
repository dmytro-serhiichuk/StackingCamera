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

    var focusRect = RectF()
        private set

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
    private var isDisplayed = false

    var isCloseToRightSide = false
        private set

    fun showFocusAt(x: Float, y: Float, size: Float) {
        currentPaint = paint
        isDisplayed = true
        isCloseToRightSide = false

        var indicatorX = x
        var indicatorY = y

        val sizeHalf = size / 2

        if (indicatorX - sizeHalf < sizeHalf) {
            indicatorX = size
        } else if (indicatorX + sizeHalf > width - sizeHalf) {
            indicatorX = width - size
            isCloseToRightSide = true
        }

        if (indicatorY - sizeHalf < sizeHalf) {
            indicatorY = size
        } else if (indicatorY + sizeHalf > height - sizeHalf) {
            indicatorY = height - size
        }

        focusRect.left   = indicatorX - sizeHalf
        focusRect.right  = indicatorX + sizeHalf
        focusRect.top    = indicatorY - sizeHalf
        focusRect.bottom = indicatorY + sizeHalf

        invalidate()
    }

    fun hide() {
        isDisplayed = false
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if (isDisplayed) {
            canvas.drawRect(focusRect, currentPaint)
        }
    }

    fun handleFocusStateUpdated(state: Boolean) {
        currentPaint = if (state) focusedPaint else notFocusedPaint
        invalidate()
    }
}