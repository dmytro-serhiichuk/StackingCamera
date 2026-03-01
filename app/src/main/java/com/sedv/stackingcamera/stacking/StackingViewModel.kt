package com.sedv.stackingcamera.stacking

import androidx.activity.ComponentActivity
import androidx.lifecycle.ViewModel
import com.sedv.stackingcamera.PermissionHelper

class StackingViewModel : ViewModel() {
    private lateinit var _permissionHelper: PermissionHelper
    val permissionHelper get() = _permissionHelper
    val bitmapHandler = LoadedBitmapsHandler
    // TODO: move to static object
    var hasMatches = false
    // TODO: move to static object
    var hasStackedResult = false

    var state = StackingState.NOT_READY

    fun initPermissionHelper(activity: ComponentActivity) {
        _permissionHelper = PermissionHelper(activity)
    }

    fun hasEnoughBitmaps(): Boolean {
        return bitmapHandler.bitmaps.size >= 2
    }
    fun isAllBitmapsInitialized(): Boolean {
        for (bitmap in bitmapHandler.bitmaps) {
            if (!bitmap.isAnalyzed) return false
        }
        return true
    }
}

enum class StackingState {
    NOT_READY,
    IDLE,
    BUSY
}