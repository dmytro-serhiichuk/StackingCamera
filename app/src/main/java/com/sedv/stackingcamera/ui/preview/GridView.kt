package com.sedv.stackingcamera.ui.preview

import android.content.Context
import android.content.res.Resources
import android.graphics.Canvas
import android.graphics.Paint
import android.util.AttributeSet
import android.view.View
import com.sedv.stackingcamera.R

class GridView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private var desiredWidth: Int = 0
    private var desiredHeight: Int = 0

    private val paint = Paint().apply {
        color = context.resources.getColor(R.color.grid_line_color)
        strokeWidth = context.resources.getDimension(R.dimen.grid_line_width)
    }

    fun setSize(width: Int, height: Int) {
        desiredWidth = width
        desiredHeight = height
        requestLayout()
        invalidate()
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

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val widthStep = width / 3

        for (i in 1 until 3) {
            val x = (widthStep * i).toFloat()
            val y1 = 0f
            val y2 = height.toFloat()

            canvas.drawLine(x, y1, x, y2, paint)
        }

        val heightStep = height / 3

        for (i in 1 until 3) {
            val x1 = 0f
            val x2 = width.toFloat()
            val y = (heightStep * i).toFloat()

            canvas.drawLine(x1, y, x2, y, paint)
        }
    }
}