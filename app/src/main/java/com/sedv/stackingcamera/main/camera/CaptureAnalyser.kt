package com.sedv.stackingcamera.main.camera

import android.graphics.ImageFormat
import android.media.Image
import android.media.ImageReader
import android.os.Handler
import android.os.SystemClock
import com.sedv.stackingcamera.main.generalsettings.GeneralSettings

class CaptureAnalyser(
    private val cameraInfo: CameraInfo,
    private val backgroundHandler: Handler
) {
    val histogram = Histogram()
    private var _meanBrightness = 0.0
    val meanBrightness get() = _meanBrightness

    private var lastUpdateTime = 0L
    private val updateIntervalMs = 50L

    private var _imageReader: ImageReader? = null
    val surface get() = _imageReader?.surface

    init {
        val yuvFormat = cameraInfo.tryGetSupportedFormat(ImageFormat.YUV_420_888)
        yuvFormat?.let { format ->
            if (format.supportedResolutions.isNotEmpty()) {
                var size = format.supportedResolutions.find { it.width == 320 } ?: format.supportedResolutions.last()
                _imageReader = ImageReader.newInstance(
                    size.width,
                    size.height,
                    ImageFormat.YUV_420_888,
                    3
                )
            }
        }

        _imageReader?.setOnImageAvailableListener({ reader ->
            val image = reader.acquireLatestImage()
            if (image != null) {
                backgroundHandler.post {
                    val currentTime = SystemClock.elapsedRealtime()
                    if (currentTime - lastUpdateTime >= updateIntervalMs) {
                        lastUpdateTime = currentTime

                        updateMeanBrightness(image)
                        if (GeneralSettings.histogram.isActive) {
                            histogram.update(image)
                        }
                    }
                    image.close()
                }
            }
        }, backgroundHandler)
    }

    private fun updateMeanBrightness(image: Image) {
        val yBuffer = image.planes[0].buffer
        val rowStride = image.planes[0].rowStride
        val pixelStride = image.planes[0].pixelStride
        val width = image.width
        val height = image.height

        var sum = 0L
        var count = 0L

        val yBytes = ByteArray(yBuffer.remaining())
        yBuffer[yBytes]

        var offset = 0
        for (row in 0 until height) {
            var colOffset = offset
            for (col in 0 until width) {
                val y = yBytes[colOffset].toInt() and 0xFF
                sum += y
                count++
                colOffset += pixelStride
            }
            offset += rowStride
        }
        _meanBrightness = sum.toDouble() / count
    }
}