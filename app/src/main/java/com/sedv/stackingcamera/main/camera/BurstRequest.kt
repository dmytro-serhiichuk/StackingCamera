package com.sedv.stackingcamera.main.camera

import android.hardware.camera2.CaptureResult
import android.hardware.camera2.TotalCaptureResult
import android.media.Image
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import com.sedv.stackingcamera.main.camera.settings.CameraOutputFormat

class BurstRequest(
    val totalImagesLeft: Int,
    val imagesLeftInCurrentIterations: Int,
    val processedImages: Int,
    val outputFormat: CameraOutputFormat,
    val onImageReadyCallback: (Image, TotalCaptureResult, CameraOutputFormat, PhotoType) -> Unit,
    val onSequenceFinished: () -> Unit,
    val onErrorCallback: ((CameraError) -> Unit)?,
) {

    private val images = mutableMapOf<Long, Image>()
    private val captureResults = mutableMapOf<Long, TotalCaptureResult>()

    private val lock = Any()

    private var processedImageInCurrentIteration = 0

    private val backgroundThread: HandlerThread
    private val backgroundHandler: Handler

    init {
        backgroundThread = HandlerThread("CameraBurstSavingThread").also { it.start() }
        backgroundHandler = Handler(backgroundThread.looper)
    }

    fun addImageToSequence(image: Image) {
        synchronized(lock) {
            if (processedImageInCurrentIteration >= imagesLeftInCurrentIterations) {
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
            if (processedImageInCurrentIteration >= imagesLeftInCurrentIterations) return

            val timestamp = captureResult[CaptureResult.SENSOR_TIMESTAMP] ?: return

            captureResults[timestamp] = captureResult

            images[timestamp]?.let {
                processPhoto(timestamp, it, captureResult)
            }
        }
    }

    private fun processPhoto(timestamp: Long, image: Image, result: TotalCaptureResult) {
        backgroundHandler.post {
            try {
                val photoType = when (processedImages + processedImageInCurrentIteration) {
                    totalImagesLeft - 1 -> PhotoType.BURST_LAST
                    0 -> PhotoType.BURST_FIRST
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

                processedImageInCurrentIteration++

                if (processedImageInCurrentIteration == imagesLeftInCurrentIterations && images.isEmpty() && captureResults.isEmpty()) {
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