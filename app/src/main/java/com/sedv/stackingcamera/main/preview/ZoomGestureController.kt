package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.MotionEvent
import android.view.ScaleGestureDetector

class ZoomGestureController(
    private val context: Context,
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

    private var scaleFactor = 1f

    private val timeoutHandler = Handler(Looper.getMainLooper())
    private val resetZoomRunnable = Runnable {
        zoomState = false
        scaleFactor = 1f
    }

    private val gestureDetector = ScaleGestureDetector(context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
        override fun onScale(detector: ScaleGestureDetector): Boolean {
            scaleFactor = detector.scaleFactor
            return true
        }

        override fun onScaleBegin(detector: ScaleGestureDetector): Boolean {
            timeoutHandler.removeCallbacks(resetZoomRunnable)
            zoomState = true
            return super.onScaleBegin(detector)
        }

        override fun onScaleEnd(detector: ScaleGestureDetector) {
            timeoutHandler.removeCallbacks(resetZoomRunnable)
            timeoutHandler.postDelayed(resetZoomRunnable, zoomLockTimeoutMs)
        }
    })

    fun onTouchEvent(event: MotionEvent): Boolean {
        gestureDetector.onTouchEvent(event)

        onZoomChanged(scaleFactor)

        return true
    }

    fun release() {
        timeoutHandler.removeCallbacks(resetZoomRunnable)
    }
}