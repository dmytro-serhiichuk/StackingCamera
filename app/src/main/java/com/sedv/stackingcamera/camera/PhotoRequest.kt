package com.sedv.stackingcamera.camera

import android.hardware.camera2.TotalCaptureResult
import android.media.Image
import android.util.Log
import com.sedv.stackingcamera.camera.settings.CameraOutputFormat

class PhotoRequest(
    val outputFormat: CameraOutputFormat,
    val onImageReadyCallback: (Image, TotalCaptureResult, CameraOutputFormat, PhotoType) -> Unit,
    val onFinishedCallback: () -> Unit,
    val onErrorCallback: ((CameraError) -> Unit)?
) {
    private var image: Image? = null
    private var captureResult: TotalCaptureResult? = null

    private val lock = Any()

    fun setImage(image: Image) {
        synchronized(lock) {
            this.image = image
            if (captureResult != null) onReady()
        }
    }
    fun setCaptureResult(captureResult: TotalCaptureResult) {
        synchronized(lock) {
            this.captureResult = captureResult
            if (image != null) onReady()
        }
    }

    private fun onReady() {
        try {
            onImageReadyCallback(image!!, this.captureResult!!, outputFormat, PhotoType.REGULAR)
        } catch (e: Exception) {
            Log.e("Camera", "Error saving image: ${e.message}")
            onErrorCallback?.invoke(CameraError.PhotoCreatingFailed("Invalid capture result"))
        } finally {
            onFinishedCallback()
        }
    }

    fun release() {
        image?.close()
    }
}