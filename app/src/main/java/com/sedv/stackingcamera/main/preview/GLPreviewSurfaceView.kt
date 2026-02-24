package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.opengl.GLSurfaceView
import android.view.Surface

class GLPreviewSurfaceView(
    context: Context,
    initWidth: Int,
    initHeight: Int,
    onSurfaceCreated: (Surface) -> Unit,
) : GLSurfaceView(context) {

    private val renderer: GLPreviewSurfaceRenderer
    private var ratioWidth = 0
    private var ratioHeight = 0

    init {
        setEGLContextClientVersion(3)

        renderer = GLPreviewSurfaceRenderer(context, initWidth, initHeight) { surface ->
            onSurfaceCreated(surface)
        }

        setRenderer(renderer)
        renderMode = RENDERMODE_CONTINUOUSLY
    }

    fun setAspectRatio(width: Int, height: Int) {
        ratioWidth = width
        ratioHeight = height
        requestLayout()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)
        if (ratioWidth == 0 || ratioHeight == 0) {
            setMeasuredDimension(width, height)
        } else {
            val targetHeight = width * ratioHeight / ratioWidth
            setMeasuredDimension(width, targetHeight)
        }
    }

    fun setFocusModeEnabled(enabled: Boolean) {
        renderer.setFocusModeEnabled(enabled)
    }
    fun setZebraPatternEnabled(enabled: Boolean) {
        renderer.setZebraPatternEnabled(enabled)
    }

    fun updateSurfaceBufferSize(width: Int, height: Int) {
        queueEvent {
            renderer.updateSurfaceSize(width, height)
        }
    }
}