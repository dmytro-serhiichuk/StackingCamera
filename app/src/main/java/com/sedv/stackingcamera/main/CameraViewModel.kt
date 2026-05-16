package com.sedv.stackingcamera.main

import android.app.Application
import android.net.Uri
import android.util.Size
import android.view.OrientationEventListener
import android.view.Surface
import androidx.activity.ComponentActivity
import androidx.lifecycle.AndroidViewModel
import com.sedv.stackingcamera.Event
import com.sedv.stackingcamera.PermissionHelper
import com.sedv.stackingcamera.main.camera.CameraController
import com.sedv.stackingcamera.main.camera.CameraState
import com.sedv.stackingcamera.main.generalsettings.property.BaseProperty
import com.sedv.stackingcamera.main.generalsettings.FrameSize
import com.sedv.stackingcamera.main.generalsettings.GeneralPropertyType
import com.sedv.stackingcamera.main.generalsettings.GeneralSettings
import com.sedv.stackingcamera.main.preview.GhostBitmap

class CameraViewModel(application: Application) : AndroidViewModel(application) {
    private lateinit var _permissionHelper: PermissionHelper
    val permissionHelper get() = _permissionHelper

    private val _cameraController: CameraController = CameraController(application)
    val cameraController get() = _cameraController
    val activeCamera get() = _cameraController.activeCamera
    private var _previewSurface: Surface? = null
    val previewSurface get() = _previewSurface

    private var _deviceOrientation: Int = 0
    val deviceOrientation get() = _deviceOrientation

    val lastBurstPhotosUris = arrayListOf<Uri>()
    var ghostBitmap: GhostBitmap? = null

    val onCameraSwitched = Event<() -> Unit>()
    val onProgramReady = Event<() -> Unit>()

    init {
        GeneralSettings.onChanged += ::handleGeneralSettingsChanged

        onCameraSwitched.clear()
        onProgramReady.clear()
    }

    fun init(activity: ComponentActivity) {
        _permissionHelper = PermissionHelper(activity)
    }

    fun onPreviewSurfaceReady(surface: Surface) {
        _previewSurface?.release()
        _previewSurface = surface

        onProgramReady.invokeAll { it.invoke() }
        previewSurface?.let { _cameraController.setPreviewSurface(it) }
        activeCamera.open()
    }

    fun getPreviewSizeWithAspectRation(ratio: Double? = null): Size {
        val r = ratio
            ?: if (GeneralSettings.frameSize.value == FrameSize.FRAME_SIZE_4_3.value) 4.0/3.0
            else 16.0/9.0
        return cameraController.activeCamera.cameraInfo.getPreviewSizeByAspectRatio(r)
    }

    fun selectCamera(id: String) {
        if (activeCamera.cameraInfo.cameraId == id) return
        if (!_cameraController.selectCamera(id)) return

        activeCamera.cameraSettings.zoomProperty?.value = 1.0f

        onCameraSwitched.invokeAll { it.invoke() }
        activeCamera.open()
    }

    fun updateOrientation(newOrientation: Int) {
        if (newOrientation == OrientationEventListener.ORIENTATION_UNKNOWN) return

        _deviceOrientation = when (newOrientation) {
            in 45..134 -> 270
            in 135..224 -> 180
            in 225..314 -> 90
            else -> 0
        }

        _cameraController.updateDeviceOrientation(_deviceOrientation)
    }

    fun pause() {
        if (_previewSurface?.isValid == true) {
            cameraController.activeCamera.close()
        }
    }

    fun resume() {
        if (cameraController.activeCamera.currentState == CameraState.CLOSED &&
            _previewSurface?.isValid == true
        ) {
            onProgramReady.invokeAll { it.invoke() }
            cameraController.activeCamera.open()
        }
    }

    fun destroy() {
        _previewSurface?.release()
        _previewSurface = null
        cameraController.destroy()

        onCameraSwitched.clear()
        onProgramReady.clear()
    }

    private fun handleGeneralSettingsChanged(prop: BaseProperty<*>) {
        if (prop.type == GeneralPropertyType.FRAME_SIZE && activeCamera.currentState != CameraState.BUSY) {
            activeCamera.close()
            onCameraSwitched.invokeAll { it.invoke() }
            activeCamera.open()
        }
    }
}