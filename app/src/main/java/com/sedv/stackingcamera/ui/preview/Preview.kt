package com.sedv.stackingcamera.ui.preview

import android.annotation.SuppressLint
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.util.Size
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.ScaleGestureDetector
import android.widget.FrameLayout
import android.widget.TextView
import androidx.core.view.GestureDetectorCompat
import androidx.core.view.isVisible
import com.sedv.stackingcamera.viewmodels.CameraViewModel
import com.sedv.stackingcamera.camera.settings.MeteringArea
import com.sedv.stackingcamera.settings.BaseProperty
import com.sedv.stackingcamera.settings.GeneralPropertyType
import com.sedv.stackingcamera.settings.GeneralSettings
import kotlinx.coroutines.Runnable
import kotlin.math.min

@SuppressLint("ClickableViewAccessibility")
class Preview(
    private val context: Context,
    private val viewModel: CameraViewModel,
    previewSurfaceContainer: FrameLayout,
    private val histogramView: HistogramView,
    private val meteringAreaIndicator: PreviewMeteringAreaIndicator,
    private val zoomIndicator: TextView,
    private val gridView: GridView,
    private val screenIndicator: TextView,
    private val ghostImageView: GhostImageView
) {
    private val previewSurface: GLPreviewSurfaceView
    private var scaleGestureDetector: ScaleGestureDetector? = null
    private var gestureDetector: GestureDetectorCompat? = null

    private val uiHandler = Handler(Looper.getMainLooper())
    private var zoomRunnable: Runnable? = null

    init {
        viewModel.onCameraSwitched += ::handleCameraSwitched
        viewModel.onProgramReady += ::handleCameraSwitched
        GeneralSettings.onChanged += ::handleGeneralPropertyChanged

        val size = viewModel.getPreviewSizeWithAspectRation()
        previewSurface = GLPreviewSurfaceView(context, size.height, size.width) { surface ->
            uiHandler.post {
                viewModel.onPreviewSurfaceReady(surface)
            }
        }
        previewSurfaceContainer.addView(previewSurface, 0)

        previewSurface.setOnTouchListener { _, event ->
            val handledTap = gestureDetector?.onTouchEvent(event) ?: false
            val handledScale = scaleGestureDetector?.onTouchEvent(event) ?: false

            handledScale || handledTap
        }
    }

    private fun handleCameraSwitched() {
        val size = viewModel.getPreviewSizeWithAspectRation()
        Log.d("Camera", "Size: $size")

        previewSurface.setAspectRatio(size.width, size.height)
        previewSurface.updateSurfaceBufferSize(size.height, size.width)

        gridView.setSize(size.width, size.height)
        ghostImageView.setSize(size.width, size.height)

        val timerLayout = screenIndicator.layoutParams
        timerLayout.width = size.width
        timerLayout.height = size.height
        screenIndicator.layoutParams = timerLayout

        scaleGestureDetector = null
        gestureDetector = null

        zoomIndicator.isVisible = false

        viewModel.activeCamera.cameraSettings.zoomProperty?.let { zoomProperty ->
            scaleGestureDetector = ScaleGestureDetector(context, object : ScaleGestureDetector.SimpleOnScaleGestureListener() {
                override fun onScale(detector: ScaleGestureDetector): Boolean {
                    val scaleFactor = detector.scaleFactor

                    zoomProperty.value *= scaleFactor
                    zoomIndicator.text = "${zoomProperty.value}X"

                    zoomIndicator.isVisible = true

                    return true
                }

                override fun onScaleBegin(detector: ScaleGestureDetector): Boolean {
                    zoomRunnable?.let {
                        uiHandler.removeCallbacks(it)
                        zoomRunnable = null
                    }
                    return super.onScaleBegin(detector)
                }

                override fun onScaleEnd(detector: ScaleGestureDetector) {
                    zoomRunnable = Runnable { zoomIndicator.isVisible = false }
                    zoomRunnable?.let {
                        uiHandler.postDelayed(it, 2000)
                    }
                }
            })
        }

        viewModel.activeCamera.cameraSettings.meteringArea?.let { meteringArea ->
            gestureDetector = GestureDetectorCompat(context, object : GestureDetector.SimpleOnGestureListener() {
                override fun onSingleTapUp(event: MotionEvent): Boolean {
                    if (event.action == MotionEvent.ACTION_UP) {
                        meteringAreaIndicator.hide()

                        val location = IntArray(2)
                        meteringAreaIndicator.getLocationOnScreen(location)
                        val relativeX = event.rawX - location[0]
                        val relativeY = event.rawY - location[1]

                        val size = Size(
                            previewSurface.width,
                            previewSurface.height
                        )
                        meteringArea.setArea(
                            relativeX,
                            relativeY,
                            size
                        )

                        val smallerSize = min(size.width, size.height)
                        meteringAreaIndicator.showFocusAt(relativeX, relativeY, smallerSize * MeteringArea.WIDTH_FRACTION)
                    }

                    return true
                }
            })
        }

        viewModel.activeCamera.captureAnalyser.histogram.onUpdated = ::onHistogramUpdated
    }

    private fun handleGeneralPropertyChanged(prop: BaseProperty<*>) {
        if (prop.type == GeneralPropertyType.HISTOGRAM) {
            histogramView.isVisible = prop.value as Boolean
        } else if (prop.type == GeneralPropertyType.GRID) {
            gridView.isVisible = prop.value as Boolean
        } else if (prop.type == GeneralPropertyType.FOCUS_PEAKING) {
            previewSurface.setFocusModeEnabled(prop.value as Boolean)
        } else if (prop.type == GeneralPropertyType.ZEBRA_PATTERN) {
            previewSurface.setZebraPatternEnabled(prop.value as Boolean)
        } else if (prop.type == GeneralPropertyType.FRAME_SIZE) {
            meteringAreaIndicator.hide()
        }
    }

    private fun onHistogramUpdated(red: IntArray, green: IntArray, blue: IntArray, maximum: Int) {
        uiHandler.post {
            histogramView.onHistogramUpdated(red, green, blue, maximum)
            histogramView.invalidate()
        }
    }
}