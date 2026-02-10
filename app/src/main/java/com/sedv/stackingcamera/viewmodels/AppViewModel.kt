package com.sedv.stackingcamera.viewmodels

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import com.sedv.stackingcamera.PermissionHelper

class AppViewModel(private val application: Application) : AndroidViewModel(application) {
    val stackingViewModel = SharedData.stackingViewModel
    val cameraViewModel = CameraViewModel(application)
    lateinit var permissionHelper: PermissionHelper

    fun init(permissionHelper: PermissionHelper) {
        this.permissionHelper = permissionHelper
    }
}