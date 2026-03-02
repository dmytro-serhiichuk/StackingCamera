package com.sedv.stackingcamera.stacking

import androidx.activity.ComponentActivity
import androidx.lifecycle.ViewModel
import com.sedv.stackingcamera.PermissionHelper

class StackingViewModel : ViewModel() {
    private lateinit var _permissionHelper: PermissionHelper
    val permissionHelper get() = _permissionHelper

    fun initPermissionHelper(activity: ComponentActivity) {
        _permissionHelper = PermissionHelper(activity)
    }
}