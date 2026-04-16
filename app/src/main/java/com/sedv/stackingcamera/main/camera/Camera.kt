package com.sedv.stackingcamera.main.camera

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.hardware.camera2.CameraAccessException
import android.hardware.camera2.CameraCaptureSession
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraDevice
import android.hardware.camera2.CameraManager
import android.hardware.camera2.CameraMetadata
import android.hardware.camera2.CaptureFailure
import android.hardware.camera2.CaptureRequest
import android.hardware.camera2.CaptureResult
import android.hardware.camera2.DngCreator
import android.hardware.camera2.TotalCaptureResult
import androidx.exifinterface.media.ExifInterface
import android.media.Image
import android.os.Handler
import android.os.SystemClock
import android.util.Log
import android.view.Surface
import androidx.core.app.ActivityCompat
import com.sedv.stackingcamera.Event
import com.sedv.stackingcamera.main.camera.settings.AEState
import com.sedv.stackingcamera.main.camera.settings.CameraOutputFormat
import com.sedv.stackingcamera.main.camera.settings.CameraSettings
import com.sedv.stackingcamera.main.generalsettings.FrameSize
import com.sedv.stackingcamera.main.generalsettings.GeneralSettings
import java.io.ByteArrayOutputStream
import kotlin.math.max
import kotlin.math.min

class Camera(
    val cameraInfo: CameraInfo,
    private val context: Context,
    private var previewSurface: Surface?,
    private val backgroundHandler: Handler,
    private val cameraManager: CameraManager,
    private var deviceOrientation: Int = 0
) {
    val cameraSettings = CameraSettings(cameraInfo, ::onSettingsChangedManually)
    private var _cameraDevice: CameraDevice? = null
    private var captureSession: CameraCaptureSession? = null
    private val imageReaders = arrayListOf<FormatImageReader>()
    private var previewRequestBuilder: CaptureRequest.Builder? = null
    private var photoRequestBuilder: CaptureRequest.Builder? = null
    private var _currentState = CameraState.CLOSED
    val currentState get() = _currentState
    private var lastSettingsUpdateTime = 0L
    private val updateIntervalMs = 100L

    var onPhotoReceived = Event<(() -> Unit)>()
    var onPhotoCreated: ((ByteArray, CameraOutputFormat, PhotoType, Int) -> Unit)? = null
    var onPhotoCreatingFailed: ((CameraError) -> Unit)? = null

    var onSettingsAutoChanged = Event<(() -> Unit)>()

    val captureAnalyser = CaptureAnalyser(cameraInfo, backgroundHandler)

    fun updatePreviewSurface(surface: Surface) {
        previewSurface = surface
        if (_cameraDevice != null && currentState == CameraState.OPENED) {
            recreateSession()
        }
    }
    fun updateDeviceOrientation(orientation: Int) {
        deviceOrientation = orientation
    }

    fun open() {
        try {
            if (ActivityCompat.checkSelfPermission(
                    context,
                    Manifest.permission.CAMERA
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                Log.e(LOG_TAG, "Camera permission not granted")
                throw CameraError.OpeningError("Camera permission not granted")
            }

            cameraManager.openCamera(cameraInfo.cameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(device: CameraDevice) {
                    Log.d(LOG_TAG, "Camera opened: ${cameraInfo.cameraId}")
                    _cameraDevice = device

                    checkSemiAutoSupport()

                    setupImageReaders()
                    recreateSession()
                    _currentState = CameraState.OPENED
                }
                override fun onDisconnected(device: CameraDevice) {
                    Log.d(LOG_TAG, "Camera disconnected: ${cameraInfo.cameraId}")
                    close()
                }
                override fun onError(device: CameraDevice, error: Int) {
                    Log.e(LOG_TAG, "Camera error: $error for camera ${cameraInfo.cameraId}")
                    close()
                    throw CameraError.OpeningError("Camera error: $error for camera ${cameraInfo.cameraId}")
                }
            }, backgroundHandler)
        } catch (e: CameraAccessException) {
            Log.e(LOG_TAG, "Failed to open camera: ${e.message}")
            throw CameraError.OpeningError("Failed to open camera: ${e.message}")
        }
    }

    private fun checkSemiAutoSupport() {
        val device = _cameraDevice ?: return

        if (!cameraInfo.hasManualSensor || cameraSettings.iso == null || cameraSettings.exposureTimeNS == null) {
            cameraInfo.supportSemiAutoExposure = false
            return
        }

        try {
            val testRequest = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            testRequest[CaptureRequest.CONTROL_AE_MODE] = CaptureRequest.CONTROL_AE_MODE_OFF
            testRequest[CaptureRequest.SENSOR_SENSITIVITY] = 0
            testRequest[CaptureRequest.SENSOR_EXPOSURE_TIME] = cameraInfo.exposureRange.lower

            val testRequest2 = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            testRequest2[CaptureRequest.CONTROL_AE_MODE] = CaptureRequest.CONTROL_AE_MODE_OFF
            testRequest2[CaptureRequest.SENSOR_SENSITIVITY] = cameraInfo.isoRange.lower
            testRequest2[CaptureRequest.SENSOR_EXPOSURE_TIME] = 0
            testRequest2[CaptureRequest.SENSOR_FRAME_DURATION] = 0

            cameraInfo.supportSemiAutoExposure = true
            Log.i(LOG_TAG, "Semi-auto exposure supported on this device")

        } catch (e: IllegalArgumentException) {
            cameraInfo.supportSemiAutoExposure = false
            Log.i(LOG_TAG, "Semi-auto exposure not supported on this device: ${e.message}")
        } catch (e: Exception) {
            cameraInfo.supportSemiAutoExposure = false
            Log.i(LOG_TAG, "Semi-auto exposure not supported, unknown error: ${e.message}")
        }
    }

    private fun setupImageReaders() {
        val maxImages = if (cameraSettings.burstProperty != null) CameraSettings.Companion.MAX_BURST_IMAGES else 2

        val jpegReader = setupJpegReader(maxImages)
        if (jpegReader != null) imageReaders.add(jpegReader)

        val rawReader = setupRAWReader(maxImages)
        if (rawReader != null) imageReaders.add(rawReader)
    }
    private fun setupJpegReader(maxImages: Int): FormatImageReader? {
        val cameraFormat = cameraInfo.supportedFormats.find { it.format == CameraOutputFormat.JPEG.value } ?: return null

        val ratio = if (GeneralSettings.frameSize.value == FrameSize.FRAME_SIZE_4_3.value) 4.0/3.0
            else 16.0/9.0

        val size = cameraFormat.getHighestResolutionByAspectRation(ratio) ?: return null
        return FormatImageReader(CameraOutputFormat.JPEG, size.width, size.height, maxImages)
    }
    private fun setupRAWReader(maxImages: Int): FormatImageReader? {
        if (!cameraInfo.hasRawCapture) return null
        val cameraFormat = cameraInfo.supportedFormats.find { it.format == CameraOutputFormat.RAW.value } ?: return null
        val size = cameraFormat.supportedResolutions.getOrNull(0) ?: return null
        return FormatImageReader(CameraOutputFormat.RAW, size.width, size.height, maxImages)
    }

    private fun recreateSession() {
        val device = _cameraDevice
        val surface = previewSurface

        if (device == null) {
            Log.e(LOG_TAG, "Camera device is not initialized")
            throw CameraError.SessionError("Camera device is not initialized")
        }
        if (surface == null || !surface.isValid) {
            Log.e(LOG_TAG, "Preview surface is null or invalid")
            throw CameraError.SessionError("Preview surface is null or invalid")
        }

        try {
            captureSession?.close()
            captureSession = null

            @Suppress("DEPRECATION")
            device.createCaptureSession(
                listOf(surface, *imageReaders.map { it.surface }.toTypedArray(), captureAnalyser.surface),
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigureFailed(session: CameraCaptureSession) {
                        Log.e(LOG_TAG, "Session configure failed;")
                        throw CameraError.SessionError("Session configure failed")
                    }

                    override fun onConfigured(session: CameraCaptureSession) {
                        captureSession = session
                        startPreview()
                        Log.i(LOG_TAG, "Session configured successfully")
                    }
                }, backgroundHandler
            )
        } catch (e: CameraAccessException) {
            Log.e(LOG_TAG, "Failed to create capture session: ${e.message}")
            throw CameraError.SessionError("Failed to create capture session: ${e.message}")
        } catch (e: IllegalStateException) {
            Log.e(LOG_TAG, "Camera device in invalid state: ${e.message}")
            throw CameraError.SessionError("Camera device in invalid state: ${e.message}")
        }
    }

    private fun startPreview() {
        val device = _cameraDevice
        val surface = previewSurface
        val session = captureSession

        if (device == null || session == null || surface == null) {
            val message = "Camera not ready for preview"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }

        try {
            previewRequestBuilder = device.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            setCaptureRequestSettings(previewRequestBuilder!!, surface)

            runPreview()

        } catch (e: CameraAccessException) {
            Log.e(LOG_TAG, "Failed to create capture session: ${e.message}")
            throw CameraError.SessionError("Failed to create capture session: ${e.message}")
        } catch (e: IllegalStateException) {
            Log.e(LOG_TAG, "Camera device in invalid state: ${e.message}")
            throw CameraError.SessionError("Camera device in invalid state: ${e.message}")
        }
    }

    private fun onSettingsChangedManually() {
        if (_currentState == CameraState.OPENED && _cameraDevice != null && captureSession != null) {
            startPreview()
        }
    }

    private fun setCaptureRequestSettings(requestBuilder: CaptureRequest.Builder, target: Surface) {
        requestBuilder.apply {
            addTarget(target)
            if (requestBuilder != photoRequestBuilder && captureAnalyser.surface != null) {
                addTarget(captureAnalyser.surface!!)
            }

            set(CaptureRequest.CONTROL_MODE, CameraMetadata.CONTROL_MODE_AUTO)
            set(CaptureRequest.JPEG_ORIENTATION, getOrientation())

            setIsoSettings(this)
            setExposureTimeSettings(this)

            val aeState = when (cameraSettings.aeState) {
                AEState.AUTO -> CaptureRequest.CONTROL_AE_MODE_ON
                else -> CaptureRequest.CONTROL_AE_MODE_OFF
            }
            set(CaptureRequest.CONTROL_AE_MODE, aeState)

            cameraSettings.ev?.let { ev ->
                set(CaptureRequest.CONTROL_AE_EXPOSURE_COMPENSATION, ev.value)
            }

            setFocusSettings(this)

            set(CaptureRequest.CONTROL_AWB_MODE,
                cameraSettings.whiteBalance?.value ?: CaptureRequest.CONTROL_AWB_MODE_AUTO)

            cameraSettings.zoomProperty?.let {
                set(CaptureRequest.SCALER_CROP_REGION, cameraSettings.zoomProperty.rect)
            }
        }
    }
    private fun setIsoSettings(requestBuilder: CaptureRequest.Builder) {
        val iso = cameraSettings.iso ?: return
        val isUseAutoMode = iso.isInAutoMode
                && requestBuilder != photoRequestBuilder
                && cameraInfo.supportSemiAutoExposure

        requestBuilder[CaptureRequest.SENSOR_SENSITIVITY] = if (isUseAutoMode) 0 else iso.value
    }
    private fun setExposureTimeSettings(requestBuilder: CaptureRequest.Builder) {
        val shutter = cameraSettings.exposureTimeNS ?: return
        val isInAutoMode = shutter.isInAutoMode
                && requestBuilder != photoRequestBuilder
                && cameraInfo.supportSemiAutoExposure

        when {
            isInAutoMode -> {
                requestBuilder[CaptureRequest.SENSOR_EXPOSURE_TIME] = 0
                requestBuilder[CaptureRequest.SENSOR_FRAME_DURATION] = 0
            }
            requestBuilder == photoRequestBuilder -> {
                requestBuilder[CaptureRequest.SENSOR_EXPOSURE_TIME] = shutter.value
                requestBuilder[CaptureRequest.SENSOR_FRAME_DURATION] = shutter.value
            }
            else -> {
                val previewExposureTime = min(shutter.value, MAX_PREVIEW_EXPOSURE_TIME)
                val previewFrameDuration = max(previewExposureTime, MAX_PREVIEW_FRAME_DURATION)
                requestBuilder[CaptureRequest.SENSOR_EXPOSURE_TIME] = previewExposureTime
                requestBuilder[CaptureRequest.SENSOR_FRAME_DURATION] = previewFrameDuration
            }
        }
    }
    private fun setFocusSettings(requestBuilder: CaptureRequest.Builder) {
        requestBuilder[CaptureRequest.CONTROL_AF_MODE] = CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE

        val focusMode = cameraSettings.focusModes ?: return
        requestBuilder[CaptureRequest.CONTROL_AF_MODE] = focusMode.value
        if (focusMode.value == CaptureRequest.CONTROL_AF_MODE_OFF) {
            cameraSettings.manualFocus?.let {
                requestBuilder[CaptureRequest.LENS_FOCUS_DISTANCE] = it.value
            }
        }

        setMeteringAreaSettings(requestBuilder)
    }
    private fun setMeteringAreaSettings(requestBuilder: CaptureRequest.Builder) {
        val meteringArea = cameraSettings.meteringArea ?: return
        if (meteringArea.isTriggered) {
            requestBuilder[CaptureRequest.CONTROL_AF_MODE] = CaptureRequest.CONTROL_AF_MODE_AUTO
            requestBuilder[CaptureRequest.CONTROL_AF_REGIONS] = meteringArea.regions
            if (meteringArea.supportAE) requestBuilder[CaptureRequest.CONTROL_AE_REGIONS] = meteringArea.regions
            if (meteringArea.supportAWB) requestBuilder[CaptureRequest.CONTROL_AWB_REGIONS] = meteringArea.regions

            requestBuilder[CaptureRequest.CONTROL_AF_TRIGGER] = CameraMetadata.CONTROL_AF_TRIGGER_START
            meteringArea.clear()
        } else {
            requestBuilder[CaptureRequest.CONTROL_AF_TRIGGER] = CameraMetadata.CONTROL_AF_TRIGGER_CANCEL
        }
    }

    private fun runPreview() {
        val session = captureSession
        if (session == null) {
            val message = "Camera not ready for preview"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }

        val captureCallback = object : CameraCaptureSession.CaptureCallback() {
            override fun onCaptureCompleted(
                session: CameraCaptureSession,
                request: CaptureRequest,
                result: TotalCaptureResult
            ) {
                super.onCaptureCompleted(session, request, result)
                handlePreviewCaptureResult(result)
            }
        }

        session.setRepeatingRequest(
            previewRequestBuilder!!.build(),
            captureCallback,
            backgroundHandler
        )
    }
    private fun handlePreviewCaptureResult(result: TotalCaptureResult) {
        val currentTime = SystemClock.elapsedRealtime()
        if (currentTime - lastSettingsUpdateTime < updateIntervalMs) return
        lastSettingsUpdateTime = currentTime

        cameraSettings.iso?.let {
            val iso = result[CaptureResult.SENSOR_SENSITIVITY]
            if (iso != null && cameraSettings.iso.isInAutoMode) {
                cameraSettings.iso.setValueWithoutNotifying(iso)
            }
        }
        cameraSettings.exposureTimeNS?.let {
            val shutterSpeed = result[CaptureResult.SENSOR_EXPOSURE_TIME]
            if (shutterSpeed != null && cameraSettings.exposureTimeNS.isInAutoMode) {
                cameraSettings.exposureTimeNS.setValueWithoutNotifying(shutterSpeed)
            }
        }

        cameraSettings.meteringArea?.let {
            val state = result[CaptureResult.CONTROL_AF_STATE]
            if (state != null && (state == CaptureResult.CONTROL_AF_STATE_FOCUSED_LOCKED || state == CaptureResult.CONTROL_AF_STATE_NOT_FOCUSED_LOCKED)) {
                onSettingsChangedManually()
            }
        }

        cameraSettings.manualFocus?.let {
            val currentFocusDistance = result[CaptureResult.LENS_FOCUS_DISTANCE]
            if (currentFocusDistance != null && currentFocusDistance >= 0f) {
                cameraSettings.manualFocus.setValueWithoutNotifying(currentFocusDistance)
            }
        }

        onSettingsAutoChanged.invokeAll { it.invoke() }
    }

    private fun getOrientation(): Int {
        val sensorOrientation = cameraInfo.sensorOrientation

        return if (cameraInfo.facing == CameraCharacteristics.LENS_FACING_FRONT) {
            (sensorOrientation + deviceOrientation) % 360
        } else {
            (sensorOrientation - deviceOrientation + 360) % 360
        }
    }
    private fun getRawOrientation(): Int {
        val orientation = getOrientation()

        return when(orientation) {
            0 -> ExifInterface.ORIENTATION_NORMAL
            90 -> ExifInterface.ORIENTATION_ROTATE_90
            180 -> ExifInterface.ORIENTATION_ROTATE_180
            else -> ExifInterface.ORIENTATION_ROTATE_270
        }
    }

    fun takePicture() {
        val device = _cameraDevice
        val session = captureSession
        val format = cameraSettings.format?.value ?: CameraOutputFormat.JPEG
        val reader = imageReaders.find { it.format == format }

        if (device == null || session == null) {
            val message = "Camera not ready for capture"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }
        if (reader == null) {
            val message = "Camera does not support capturing in ${format.displayName}"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }

        if (_currentState == CameraState.BUSY) return
        _currentState = CameraState.BUSY

        try {
            photoRequestBuilder = device
                .createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)

            setCaptureRequestSettings(photoRequestBuilder!!, reader.surface)

            val photoRequest = PhotoRequest(
                format,
                ::onImageReadyCallback,
                ::deblockCamera,
                onPhotoCreatingFailed
            )

            reader.setOnImageAvailableListener({ reader ->
                val image = reader.acquireLatestImage()
                if (image != null) {
                    onPhotoReceived.invokeAll { it.invoke() }
                    backgroundHandler.post {
                        photoRequest.setImage(image)
                    }
                }
            }, backgroundHandler)

            val captureCallback = object : CameraCaptureSession.CaptureCallback() {
                override fun onCaptureCompleted(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    result: TotalCaptureResult
                ) {
                    Log.d(LOG_TAG, "Picture capture completed")
                    photoRequest.setCaptureResult(result)
                }

                override fun onCaptureFailed(
                    session: CameraCaptureSession,
                    request: CaptureRequest,
                    failure: CaptureFailure
                ) {
                    Log.e(LOG_TAG, "Picture capture failed: ${failure.reason}")
                    photoRequest.release()
                }
            }

            session.capture(
                photoRequestBuilder!!.build(), 
                captureCallback, 
                backgroundHandler
            )

        } catch (e: CameraAccessException) {
            val message = "Failed to capture session: ${e.message}"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        } catch (e: IllegalStateException) {
            val message = "Camera device in invalid state: ${e.message}"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }
    }
    fun takeBurst() {
        val device = _cameraDevice
        val session = captureSession
        val format = cameraSettings.format?.value ?: CameraOutputFormat.JPEG
        val burstProperty = cameraSettings.burstProperty

        if (device == null || session == null) {
            val message = "Camera not ready for capture"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }
        if (burstProperty == null) {
            val message = "Camera does not support burst mode"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }

        val reader = imageReaders.find { it.format == format }

        if (reader == null) {
            val message = "Camera does not support capturing in ${format.displayName}"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }

        if (_currentState == CameraState.BUSY) return
        _currentState = CameraState.BUSY

        try {
            captureBurstIteration(burstProperty.value, device, reader, format, session)
        } catch (e: CameraAccessException) {
            val message = "Failed to capture session: ${e.message}"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        } catch (e: IllegalStateException) {
            val message = "Camera device in invalid state: ${e.message}"
            Log.e(LOG_TAG, message)
            throw CameraError.SessionError(message)
        }
    }

    private fun captureBurstIteration(
        frames: Int,
        device: CameraDevice,
        reader: FormatImageReader,
        format: CameraOutputFormat,
        session: CameraCaptureSession
    ) {
        val nextIterationFrames = max(frames - CameraSettings.Companion.MAX_BURST_IMAGES, 0)
        val currentFrames = frames - nextIterationFrames

        photoRequestBuilder = device
            .createCaptureRequest(CameraDevice.TEMPLATE_STILL_CAPTURE)

        setCaptureRequestSettings(photoRequestBuilder!!, reader.surface)

        val burstRequest = BurstRequest(
            frames,
            currentFrames,
            format,
            ::onImageReadyCallback,
            {
                if (nextIterationFrames > 0) {
                    captureBurstIteration(nextIterationFrames, device, reader, format, session)
                } else {
                    deblockCamera()
                }
            },
            onPhotoCreatingFailed
        )

        reader.setOnImageAvailableListener({ reader ->
            val image = reader.acquireLatestImage()
            if (image != null) {
                onPhotoReceived.invokeAll { it.invoke() }

                burstRequest.addImageToSequence(image)
            }
        }, backgroundHandler)

        val captureCallback = object : CameraCaptureSession.CaptureCallback() {
            override fun onCaptureCompleted(
                session: CameraCaptureSession,
                request: CaptureRequest,
                result: TotalCaptureResult
            ) {
                Log.d(LOG_TAG, "Picture capture completed")
                burstRequest.addCaptureResultToSequence(result)
            }

            override fun onCaptureFailed(
                session: CameraCaptureSession,
                request: CaptureRequest,
                failure: CaptureFailure
            ) {
                Log.e(LOG_TAG, "Picture capture failed: ${failure.reason}")
                burstRequest.release()
            }
        }

        val captureRequests = mutableListOf<CaptureRequest>()
        repeat(currentFrames) {
            captureRequests.add(photoRequestBuilder!!.build())
        }

        session.captureBurst(
            captureRequests,
            captureCallback,
            backgroundHandler
        )
    }

    private fun onImageReadyCallback(image: Image, captureResult: TotalCaptureResult, format: CameraOutputFormat, photoType: PhotoType) {
        val orientation = getOrientation()

        if (format == CameraOutputFormat.JPEG) {
            val buffer = image.planes[0].buffer
            val bytes = ByteArray(buffer.capacity())
            buffer[bytes]

            onPhotoCreated?.invoke(bytes, format, photoType, orientation)
        }
        else if (format == CameraOutputFormat.RAW) {
            val baos = ByteArrayOutputStream()
            val characteristics = cameraManager.getCameraCharacteristics(cameraInfo.cameraId)

            DngCreator(characteristics, captureResult)
                .setOrientation(getRawOrientation())
                .use { dng ->
                    dng.writeImage(baos, image)
                }
            onPhotoCreated?.invoke(baos.toByteArray(), format, photoType, orientation)
        }
    }

    private fun deblockCamera() {
        _currentState = CameraState.OPENED
    }

    fun close() {
        if (_currentState == CameraState.BUSY) return

        try {
            captureSession?.close().also { captureSession = null }
            _cameraDevice?.close().also { _cameraDevice = null }
            imageReaders.forEach { it.close() }
            imageReaders.clear()
            onPhotoReceived.clear()
            onPhotoCreated = null
            onPhotoCreatingFailed = null
            onSettingsAutoChanged.clear()
            Log.d(LOG_TAG, "Camera closed: ${cameraInfo.cameraId}")

            _currentState = CameraState.CLOSED
        } catch (e: Exception) {
            Log.e(LOG_TAG, "Error closing camera: ${e.message}")
            throw CameraError.ClosingError("Error closing camera: ${e.message}")
        }
    }
    
    companion object {
        const val LOG_TAG = "Camera"
        const val MAX_PREVIEW_EXPOSURE_TIME = 100_000_000L
        const val MAX_PREVIEW_FRAME_DURATION = 30_303_030L
    }
}