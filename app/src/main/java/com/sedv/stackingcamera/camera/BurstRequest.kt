package com.sedv.stackingcamera.camera

import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.media.Image
import android.os.Handler
import android.os.HandlerThread
import android.os.Looper
import android.util.Log
import com.sedv.stackingcamera.camera.settings.CameraOutputFormat
import kotlin.collections.get

class BurstRequest(
    val number: Int,
    val outputFormat: CameraOutputFormat,
    val onImageReadyCallback: (Image, TotalCaptureResult, CameraOutputFormat, PhotoType) -> Unit,
    val onSequenceFinished: () -> Unit,
    val onErrorCallback: ((CameraError) -> Unit)?,
) {

    private val images = mutableMapOf<Long, Image>()
    private val captureResults = mutableMapOf<Long, TotalCaptureResult>()

    private val lock = Any()

    private var processedCount = 0

    private val backgroundThread: HandlerThread
    private val backgroundHandler: Handler

    init {
        backgroundThread = HandlerThread("CameraBurstSavingThread").also { it.start() }
        backgroundHandler = Handler(backgroundThread.looper)
    }

    fun addImageToSequence(image: Image) {
        synchronized(lock) {
            if (processedCount >= number) {
                image.close()
                return
            }

            images[image.timestamp] = image

            captureResults[image.timestamp]?.let {
                processPhoto(image.timestamp, image, it)
            }
        }
    }

    fun addCaptureResultToSequence(captureResult: TotalCaptureResult) {
        synchronized(lock) {
            if (processedCount >= number) return

            val timestamp = captureResult.get(CaptureResult.SENSOR_TIMESTAMP) ?: return

            captureResults[timestamp] = captureResult

            images[timestamp]?.let {
                processPhoto(timestamp, it, captureResult)
            }
        }
    }

    private fun processPhoto(timestamp: Long, image: Image, result: TotalCaptureResult) {
        backgroundHandler.post {
            try {
                val photoType = when (processedCount) {
                    0 -> PhotoType.BURST_FIRST
                    number - 1 -> PhotoType.BURST_LAST
                    else -> PhotoType.BURST_REGULAR
                }
                onImageReadyCallback(image, result, outputFormat, photoType)
            } catch (e: Exception) {
                Log.e("Camera", "Error saving image: ${e.message}")
                onErrorCallback?.invoke(CameraError.PhotoCreatingFailed("Invalid capture result"))
            } finally {
                image.close()
                Log.d("Camera", "Image closed")

                images.remove(timestamp)
                captureResults.remove(timestamp)

                processedCount++

                if (processedCount == number && images.isEmpty() && captureResults.isEmpty()) {
                    onSequenceFinished()
                    backgroundThread.quitSafely()
                }
            }
        }
    }

    fun release() {
        images.values.forEach { it.close() }
        images.clear()
        captureResults.clear()
        backgroundThread.quitSafely()
    }
}