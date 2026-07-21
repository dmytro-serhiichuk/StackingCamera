package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import com.sedv.stackingcamera.R

class ZoomGestureController(
    private val context: Context,
    private val minZoom: Float,
    private val maxZoom: Float,
    private val onZoomStateChanged: (Boolean) -> Unit,
    private val onZoomChanged: (Float) -> Unit,
    private val zoomLockTimeoutMs: Long = 2000L
) {
    private var zoomState = false
        set(value) {
            if (field != value) {
                field = value
                onZoomStateChanged(value)
            }
        }

    private val pxPerZoomUnit = context.resources.getDimension(R.dimen.zoom_px_per_unit)

    private var zoomAtGestureStart = 1f
    private var spanAtGestureStart = 0f
    private var currentZoom = 1f

    private val timeoutHandler = Handler(Looper.getMainLooper())
    private val resetZoomRunnable = Runnable {
        zoomState = false
    }

    private val gestureDetector = ScaleGestureDetector(context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScale(detector: ScaleGestureDetector): Boolean {
            val spanDelta = detector.currentSpan - spanAtGestureStart
            val zoomDelta = spanDelta / pxPerZoomUnit

            val newZoom = (zoomAtGestureStart + zoomDelta).coerceIn(minZoom, maxZoom)
            currentZoom = newZoom
            return true
        }

        override fun onScaleBegin(detector: ScaleGestureDetector): Boolean {
            timeoutHandler.removeCallbacks(resetZoomRunnable)
            zoomState = true
            zoomAtGestureStart = currentZoom
            spanAtGestureStart = detector.currentSpan
            return super.onScaleBegin(detector)
        }

        override fun onScaleEnd(detector: ScaleGestureDetector) {
            timeoutHandler.removeCallbacks(resetZoomRunnable)
            timeoutHandler.postDelayed(resetZoomRunnable, zoomLockTimeoutMs)
        }
    })

    fun onTouchEvent(event: MotionEvent): Boolean {
        gestureDetector.onTouchEvent(event)

        onZoomChanged(currentZoom)

        return true
    }

    fun release() {
        timeoutHandler.removeCallbacks(resetZoomRunnable)
    }
}