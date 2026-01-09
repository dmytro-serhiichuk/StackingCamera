package com.sedv.stackingcamera.ui.preview

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.PorterDuff
import android.graphics.PorterDuffXfermode
import android.util.AttributeSet
import android.view.View

class HistogramView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {
    private var red   = IntArray(256)
    private var green = IntArray(256)
    private var blue  = IntArray(256)

    private var maximum: Int = 0

    private val redPaint = Paint().apply {
        color = Color.RED
        xfermode = PorterDuffXfermode(PorterDuff.Mode.ADD)
    }
    private val greenPaint = Paint().apply {
        color = Color.GREEN
        xfermode = PorterDuffXfermode(PorterDuff.Mode.ADD)
    }
    private val bluePaint = Paint().apply {
        color = Color.BLUE
        xfermode = PorterDuffXfermode(PorterDuff.Mode.ADD)
    }

    fun onHistogramUpdated(red: IntArray, green: IntArray, blue: IntArray, maximum: Int) {
        this.red = red.copyOf()
        this.green = green.copyOf()
        this.blue = blue.copyOf()

        this.maximum = maximum
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val layerId = canvas.saveLayer(0f, 0f, width.toFloat(), height.toFloat(), null)

        if (maximum == 0) {
            canvas.restoreToCount(layerId)
            return
        }

        val lineWidth = width / 256.0f

        redPaint.strokeWidth = lineWidth
        greenPaint.strokeWidth = lineWidth
        bluePaint.strokeWidth = lineWidth

        for (i in 0 until 255) {
            val x = i * lineWidth
            val y1 = height.toFloat()
            val y0r = (height - height * (red[i].toDouble() / maximum)).toFloat()
            val y0g = (height - height * (green[i].toDouble() / maximum)).toFloat()
            val y0b = (height - height * (blue[i].toDouble() / maximum)).toFloat()

            canvas.drawLine(x, y0r, x, y1, redPaint)
            canvas.drawLine(x, y0g, x, y1, greenPaint)
            canvas.drawLine(x, y0b, x, y1, bluePaint)
        }

        canvas.restoreToCount(layerId)
    }

    fun rotate(deviceOrientation: Int) {
        rotation = deviceOrientation.toFloat()
        when (deviceOrientation) {
            90, 270 -> {
                val offset = (width - height) / 2f
                translationX = offset
                translationY = offset
            }
            else -> {
                translationX = 0f
                translationY = 0f
            }
        }
    }
}