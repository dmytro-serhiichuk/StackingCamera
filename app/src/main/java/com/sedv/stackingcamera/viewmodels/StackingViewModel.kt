package com.sedv.stackingcamera.viewmodels

import com.sedv.stackingcamera.stacking.BitmapInfo

class StackingViewModel {
    val bitmaps = arrayListOf<BitmapInfo>()
    var hasStackedResult = false

    var state = StackingState.NOT_READY

    fun canStack(): Boolean {
        return bitmaps.size >= 2
    }
    fun isAllBitmapsInitialized(): Boolean {
        for (bitmap in bitmaps) {
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