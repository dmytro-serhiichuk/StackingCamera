package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.view.View
import com.sedv.stackingcamera.R

class ExposureCompensationSlider(
    context: Context,
    attrs: AttributeSet? = null
) : View(context, attrs) {

    private val backPaint = Paint().apply {
        style = Paint.Style.STROKE
        color = Color.GRAY
        strokeWidth = context.resources.getDimension(R.dimen.ev_slider_stroke_width)
        isAntiAlias = true
    }
    private val frontPaint = Paint().apply {
        style = Paint.Style.STROKE
        color = Color.WHITE
        strokeWidth = context.resources.getDimension(R.dimen.ev_slider_stroke_width)
        isAntiAlias = true
    }

    private var lineX = 0F
    private var lineY = 0F
    private var lineSize = 0F
    private var evProgress = 0F

    private var isDisplayed = false

    fun showAt(anchor: RectF, showAtLeftSide: Boolean, progress: Float) {
        lineY = anchor.top
        lineSize = anchor.right - anchor.left
        lineX = if (!showAtLeftSide) {
            anchor.right + lineSize / 4
        } else {
            anchor.left - lineSize / 4
        }
        evProgress = progress

        isDisplayed = true
        invalidate()
    }

    fun hide() {
        isDisplayed = false
        invalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        if (isDisplayed) {
            canvas.drawLine(lineX, lineY, lineX, lineY + lineSize, backPaint)
            val start = lineY + (1F - evProgress) * lineSize
            canvas.drawLine(lineX, start, lineX, lineY + lineSize, frontPaint)
        }
    }
}