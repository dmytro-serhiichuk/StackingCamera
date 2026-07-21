package com.sedv.stackingcamera.main.preview

import android.annotation.SuppressLint
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.util.Log
import android.util.Size
import android.widget.FrameLayout
import android.widget.TextView
import androidx.core.view.isVisible
import com.sedv.stackingcamera.main.CameraViewModel
import com.sedv.stackingcamera.main.camera.settings.MeteringArea
import com.sedv.stackingcamera.main.generalsettings.GeneralPropertyType
import com.sedv.stackingcamera.main.generalsettings.GeneralSettings
import com.sedv.stackingcamera.main.generalsettings.property.BaseProperty
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

    private val uiHandler = Handler(Looper.getMainLooper())

    private var zoomGestureController: ZoomGestureController? = null
    private var exposureFocusController: ExposureFocusController? = null

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
            val handledTap = exposureFocusController?.onTouchEvent(event) ?: false
            val handledScale = zoomGestureController?.onTouchEvent(event) ?: false

            handledScale || handledTap
        }
    }

    @Suppress("DEPRECATION")
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

        zoomIndicator.isVisible = false
        setupZoomGestureController()

        viewModel.activeCamera.cameraSettings.meteringArea?.let { meteringArea ->
            meteringArea.onFocusStateUpdated = meteringAreaIndicator::handleFocusStateUpdated
        }
        setupExposureFocusController()

        viewModel.activeCamera.captureAnalyser.histogram.onUpdated = ::onHistogramUpdated
    }

    private fun setupZoomGestureController() {
        zoomGestureController?.release()
        zoomGestureController = ZoomGestureController(
            context,
            minZoom = 1f,
            maxZoom = viewModel.activeCamera.cameraInfo.maxZoom,
            onZoomStateChanged = { state ->
                zoomIndicator.isVisible = state
            },
            onZoomChanged = { zoom ->
                viewModel.activeCamera.cameraSettings.zoomProperty?.let { zoomProperty ->
                    zoomProperty.value = zoom
                    zoomIndicator.text = "${zoomProperty.value}X"
                }
            }
        )
    }
    private fun setupExposureFocusController() {
        exposureFocusController?.release()
        exposureFocusController = ExposureFocusController(
            context,
            onFocus = { x, y ->
                viewModel.activeCamera.cameraSettings.meteringArea?.let { meteringArea ->
                    meteringAreaIndicator.hide()

                    val location = IntArray(2)
                    meteringAreaIndicator.getLocationOnScreen(location)
                    val relativeX = x - location[0]
                    val relativeY = y - location[1]

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
            },
            onExposureChanged = { evIndex ->
                viewModel.activeCamera.cameraSettings.ev?.setValueWithNotifying(evIndex)
            },
            onFocusLockChanged = { isLocked ->
                if (!isLocked) meteringAreaIndicator.hide()
            },
            minEv = viewModel.activeCamera.cameraInfo.evRange.lower,
            maxEv = viewModel.activeCamera.cameraInfo.evRange.upper
        )
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
            ghostImageView.handleGeneralSettingsChanged()
        } else if (prop.type == GeneralPropertyType.GHOST_IMAGE) {
            ghostImageView.handleGeneralSettingsChanged()
        }
    }

    private fun onHistogramUpdated(red: IntArray, green: IntArray, blue: IntArray, maximum: Int) {
        uiHandler.post {
            histogramView.onHistogramUpdated(red, green, blue, maximum)
            histogramView.invalidate()
        }
    }

    fun release() {
        exposureFocusController?.release()
        exposureFocusController = null
        zoomGestureController?.release()
        zoomGestureController = null
    }
}