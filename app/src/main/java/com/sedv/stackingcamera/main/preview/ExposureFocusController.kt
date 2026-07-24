package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.ViewConfiguration
import com.sedv.stackingcamera.R
import kotlin.math.abs
import kotlin.math.roundToInt

class ExposureFocusController (
    private val context: Context,
    private val onFocus: (x: Float, y: Float) -> Unit,
    private val onExposureChanged: (evIndex: Int) -> Unit,
    private val onFocusLockChanged: (isLocked: Boolean) -> Unit,
    private val minEv: Int,
    private val maxEv: Int,
    private val onExposureDragStart: () -> Unit = {},
    private val onExposureDragEnd: () -> Unit = {},
    private val focusLockTimeoutMs: Long = 5000L
) {
    private val pxPerEvStep = context.resources.getDimension(R.dimen.ev_px_per_step)
    private val touchSlop = ViewConfiguration.get(context).scaledTouchSlop

    private var focusLocked = false
        set(value) {
            if (field != value) {
                field = value
                onFocusLockChanged(value)
            }
        }

    private var focusX = 0f
    private var focusY = 0f

    private var dragStartY: Float? = null
    private var isDragging = false
    private var evIndexAtDragStart = 0
    private var currentEvIndex = 0

    private val timeoutHandler = Handler(Looper.getMainLooper())
    private val resetFocusRunnable = Runnable {
        focusLocked = false
        currentEvIndex = 0
    }

    private val gestureDetector =
        GestureDetector(context, object : GestureDetector.SimpleOnGestureListener() {
            override fun onDown(e: MotionEvent): Boolean = true

            override fun onSingleTapUp(e: MotionEvent): Boolean {
                focusX = e.x
                focusY = e.y
                currentEvIndex = 0
                focusLocked = true
                onFocus(focusX, focusY)
                return true
            }
        })

    fun onTouchEvent(event: MotionEvent): Boolean {
        gestureDetector.onTouchEvent(event)

        if (!focusLocked) return true

        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> {
                timeoutHandler.removeCallbacks(resetFocusRunnable)
                dragStartY = event.y
                isDragging = false
                evIndexAtDragStart = currentEvIndex
            }

            MotionEvent.ACTION_MOVE -> {
                val startY = dragStartY ?: return true
                val deltaY = event.y - startY

                if (!isDragging && abs(deltaY) > touchSlop) {
                    isDragging = true
                    onExposureDragStart()
                }

                if (isDragging) {
                    val deltaIndex = (-deltaY / pxPerEvStep).roundToInt()
                    currentEvIndex = (evIndexAtDragStart + deltaIndex).coerceIn(minEv, maxEv)
                    onExposureChanged(currentEvIndex)
                }
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                if (isDragging) {
                    onExposureDragEnd()
                }
                dragStartY = null
                isDragging = false

                timeoutHandler.removeCallbacks(resetFocusRunnable)
                timeoutHandler.postDelayed(resetFocusRunnable, focusLockTimeoutMs)
            }
        }
        return true
    }

    fun release() {
        timeoutHandler.removeCallbacks(resetFocusRunnable)
    }
}