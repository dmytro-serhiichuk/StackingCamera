package com.sedv.stackingcamera.camera

import android.content.Context
import android.graphics.SurfaceTexture
import android.hardware.camera2.CameraAccessException
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.hardware.camera2.params.StreamConfigurationMap
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.util.Log
import android.util.Range
import android.util.Size
import android.util.SizeF
import android.view.Surface

class CameraController(private val context: Context) {
    private val cameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
    private val _cameras = mutableMapOf<String, Camera>()
    val cameras: Map<String, Camera>

    private var _activeCamera: Camera
    val activeCamera: Camera get() = _activeCamera

    private var backgroundThread: HandlerThread? = null
    private var backgroundHandler: Handler? = null

    init {
        try {
            backgroundThread = HandlerThread("CameraBackgroundThread").also { it.start() }
            backgroundHandler = Handler(backgroundThread!!.looper)

            cameraManager.cameraIdList.forEach { cameraId ->
                val characteristics = cameraManager.getCameraCharacteristics(cameraId)
                val cameraInfo = createCameraInfo(cameraId, characteristics)
                if (cameraInfo != null) {
                    _cameras[cameraId] = Camera(
                        cameraInfo,
                        context,
                        null,
                        backgroundHandler!!,
                        cameraManager
                    )
                }
            }
        } catch (e: CameraAccessException) {
            throw CameraError.NotSupported(e.message ?: "Initializing failed")
        }

        if (_cameras.isEmpty()) {
            throw CameraError.NotSupported("Cameras not found or can not be accessed")
        }

        defineLogical()
        defineMainCamera()
        defineFront()
        defineUltraWide()
        defineTelephoto()
        defineOther()

        cameras = _cameras.toMap()
        _activeCamera = _cameras[getMainCameraId()] ?: throw CameraError.NotFound("Main camera not found")
    }

    private fun createCameraInfo(cameraId: String, characteristics: CameraCharacteristics): CameraInfo? {
        val configMap = characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)

        val physicalCameraIds = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            characteristics.physicalCameraIds
        } else {
            emptySet()
        }

        val sensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION) ?: 0

        val supportedFormats = configMap?.let { getCameraFormats(it) } ?: emptyList()
        val previewSizes = configMap?.getOutputSizes(SurfaceTexture::class.java)?.toList() ?: emptyList()

        val aeFpsRanges = characteristics
            .get(CameraCharacteristics.CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES)
            ?.toList() ?: emptyList()
        val highSpeedFpsRanges = configMap?.getHighSpeedVideoFpsRanges()?.toList() ?: emptyList()
        val hasFlash = characteristics.get(CameraCharacteristics.FLASH_INFO_AVAILABLE) ?: false
        val maxZoom = characteristics.get(CameraCharacteristics.SCALER_AVAILABLE_MAX_DIGITAL_ZOOM) ?: 1.0f

        val focalLengths = characteristics
            .get(CameraCharacteristics.LENS_INFO_AVAILABLE_FOCAL_LENGTHS)
            ?: floatArrayOf()
        val apertures = characteristics
            .get(CameraCharacteristics.LENS_INFO_AVAILABLE_APERTURES)
            ?: floatArrayOf()
        val minFocusDistance = characteristics
            .get(CameraCharacteristics.LENS_INFO_MINIMUM_FOCUS_DISTANCE) ?: 0f
        val hyperfocalDistance = characteristics
            .get(CameraCharacteristics.LENS_INFO_HYPERFOCAL_DISTANCE) ?: 0f
        val oisModes = characteristics
            .get(CameraCharacteristics.LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION)
            ?: IntArray(0)

        val rawCaps = characteristics
            .get(CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES)
            ?.toSet() ?: emptySet()

        val hasManualSensor = CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR in rawCaps
        val hasRawCapture = CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_RAW in rawCaps
        val hasBurst = CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE in rawCaps
        val hasYuvReproc = CameraCharacteristics.REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING in rawCaps

        val afModes = characteristics
            .get(CameraCharacteristics.CONTROL_AF_AVAILABLE_MODES)
            ?.toSet() ?: emptySet()
        val hasManualFocus = afModes.contains(CameraCharacteristics.CONTROL_AF_MODE_OFF) && minFocusDistance > 0f

        val lensFacing = characteristics.get(CameraCharacteristics.LENS_FACING) ?: -1

        val physicalSize = characteristics
            .get(CameraCharacteristics.SENSOR_INFO_PHYSICAL_SIZE)
            ?: SizeF(0.0f, 0.0f)

        val awbModes = characteristics.get(CameraCharacteristics.CONTROL_AWB_AVAILABLE_MODES) ?: IntArray(0)
        val arraySize = characteristics.get(CameraCharacteristics.SENSOR_INFO_PIXEL_ARRAY_SIZE) ?: Size(0, 0)

        val exposureRange = characteristics.get(CameraCharacteristics.SENSOR_INFO_EXPOSURE_TIME_RANGE) ?: Range(0L, 0L)
        val isoRange = characteristics.get(CameraCharacteristics.SENSOR_INFO_SENSITIVITY_RANGE) ?: Range(0, 0)
        val evRange = characteristics.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_RANGE) ?: Range(0, 0)
        val evStep = characteristics.get(CameraCharacteristics.CONTROL_AE_COMPENSATION_STEP)

        val colorFilter = characteristics.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT) ?: -1

        val afRegions = characteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AF) ?: 0
        val aeRegions = characteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AE) ?: 0
        val awbRegions = characteristics.get(CameraCharacteristics.CONTROL_MAX_REGIONS_AWB) ?: 0

        return CameraInfo(
            cameraId = cameraId,
            facing = lensFacing,
            sensorOrientation = sensorOrientation,
            supportedFormats = supportedFormats,
            supportedPreviewSizes = previewSizes,
            aeFpsRanges = aeFpsRanges,
            highSpeedFpsRanges = highSpeedFpsRanges,
            hasFlash = hasFlash,
            maxZoom = maxZoom,
            focalLengths = focalLengths,
            apertures = apertures,
            minFocusDistance = minFocusDistance,
            hyperfocalDistance = hyperfocalDistance,
            oisModes = oisModes,
            hasManualSensor = hasManualSensor,
            hasRawCapture = hasRawCapture,
            hasBurst = hasBurst,
            hasYuvReprocessing = hasYuvReproc,
            physicalSize = physicalSize,
            afModes = afModes,
            hasManualFocus = hasManualFocus,
            awbModes = awbModes,
            arraySize = arraySize,
            exposureRange = exposureRange,
            isoRange = isoRange,
            evRange = evRange,
            evStep = evStep,
            colorFilter = colorFilter,
            afRegions = afRegions,
            aeRegions = aeRegions,
            awbRegions = awbRegions,
            physicalCameraIds = physicalCameraIds
        )
    }

    private fun getCameraFormats(configMap: StreamConfigurationMap): List<CameraPhotoFormat> {
        return configMap.outputFormats
            .map { format -> CameraPhotoFormat(format, configMap.getOutputSizes(format).toList()) }
    }

    private fun defineLogical() {
        _cameras.values.forEach {
            if (it.cameraInfo.isLogicalCamera) {
                it.cameraInfo.cameraType = CameraType.LOGICAL
                it.cameraInfo.cameraLabel = "L"
            }
        }
    }

    private fun defineMainCamera() {
        val maxArea =
            _cameras.values.maxOfOrNull { it.cameraInfo.arraySize.width * it.cameraInfo.arraySize.height }
                ?: 0

        val largest = _cameras.filterValues {
            it.cameraInfo.arraySize.width * it.cameraInfo.arraySize.height == maxArea &&
            it.cameraInfo.cameraType == CameraType.UNDEFINED &&
            it.cameraInfo.facing == CameraCharacteristics.LENS_FACING_BACK
        }

        val main = if (largest.size == 1) {
            largest.entries.first().value.cameraInfo
        } else {
            largest.values.minBy {
                it.cameraInfo.focalLengths.getOrNull(0) ?: Float.MAX_VALUE
            }.cameraInfo
        }

        main.cameraType = CameraType.MAIN
        main.cameraLabel = "W"
    }
    private fun defineUltraWide() {
        val mainCamera = _cameras.values.find { it.cameraInfo.cameraType == CameraType.MAIN }!!.cameraInfo

        val wider = _cameras.values.filter {
            it.cameraInfo.fov > mainCamera.fov &&
            it.cameraInfo.cameraType == CameraType.UNDEFINED &&
            it.cameraInfo.facing == CameraCharacteristics.LENS_FACING_BACK
        }
        wider.sortedByDescending { it.cameraInfo.arraySize.width * it.cameraInfo.arraySize.height }
        if (wider.isNotEmpty()) {
            wider[0].cameraInfo.cameraType = CameraType.ULTRA_WIDE
            wider[0].cameraInfo.cameraLabel = "UW"
        }
    }
    private fun defineTelephoto() {
        val mainCamera = _cameras.values.find { it.cameraInfo.cameraType == CameraType.MAIN }!!.cameraInfo
        val minEfl = mainCamera.equivalentFocalLengths[0] * 1.5

        _cameras.values.forEach {
            if (it.cameraInfo.cameraType == CameraType.UNDEFINED &&
                it.cameraInfo.facing == CameraCharacteristics.LENS_FACING_BACK) {
                val efl = it.cameraInfo.equivalentFocalLengths.getOrNull(0)
                if (efl != null && efl > minEfl) {
                    it.cameraInfo.cameraType = CameraType.TELEPHOTO
                    it.cameraInfo.cameraLabel = "T"
                }
            }
        }
    }
    private fun defineFront() {
        _cameras.forEach {
            val info = it.value.cameraInfo
            if (info.facing == CameraCharacteristics.LENS_FACING_FRONT &&
                info.cameraType == CameraType.UNDEFINED
            ) {
                info.cameraType = CameraType.FRONT
                info.cameraLabel = "F"
            }
        }
    }
    private fun defineOther() {
        _cameras.forEach {
            val info = it.value.cameraInfo

            if (info.cameraType == CameraType.UNDEFINED) {
                info.cameraType = CameraType.OTHER
                info.cameraLabel = "O"
            }
        }
    }


    fun selectCamera(id: String): Boolean {
        if (_activeCamera.cameraInfo.cameraId == id) return false

        _activeCamera.close()

        if (_activeCamera.currentState != CameraState.CLOSED) return false

        _activeCamera = cameras[id] ?: throw CameraError.NotFound("Camera (id = $id) not found")
        return true
    }
    fun getMainCameraId(): String {
        val mainCamera = cameras.values.find { it.cameraInfo.cameraType == CameraType.MAIN }
        if (mainCamera != null) {
            return mainCamera.cameraInfo.cameraId
        }
        throw CameraError.NotFound("Main camera not found")
    }

    fun setPreviewSurface(surface: Surface) {
        _cameras.values.forEach { it.updatePreviewSurface(surface) }
    }

    fun destroy() {
        _activeCamera.close()
        _cameras.values.forEach { it.close() }
        backgroundThread?.quitSafely()
        try {
            backgroundThread?.join()
            backgroundThread = null
            backgroundHandler = null
        } catch (e: InterruptedException) {
            Log.e("CameraController", "Error stopping background thread", e)
        }
    }
}