package com.sedv.stackingcamera.ui.settings

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.util.AttributeSet
import android.view.View
import com.sedv.stackingcamera.R

class SliderMiddleLine(
    context: Context,
    attrs: AttributeSet? = null
) : View(context, attrs) {

    private val paint = Paint().apply {
        color = Color.RED
        strokeWidth = context.resources.getDimension(R.dimen.slider_selected_tick_width)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        val x = width / 2.0f
        canvas.drawLine(x, height * 0.5f, x, height.toFloat(), paint)
    }
}