package com.sedv.stackingcamera.viewmodels

import android.app.Application
import androidx.activity.ComponentActivity
import androidx.lifecycle.AndroidViewModel
import com.sedv.stackingcamera.PermissionHelper

class AppViewModel(application: Application) : AndroidViewModel(application) {
    val stackingViewModel = SharedData.stackingViewModel
    val cameraViewModel = CameraViewModel(application)
    private lateinit var _permissionHelper: PermissionHelper
    val permissionHelper get() = _permissionHelper

    var isReady = false

    fun init(activity: ComponentActivity) {
        if (!isReady) {
            _permissionHelper = PermissionHelper(activity)
        }
    }
}