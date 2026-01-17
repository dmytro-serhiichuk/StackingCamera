package com.sedv.stackingcamera.stacking

import androidx.lifecycle.ViewModel

class StackingViewModel : ViewModel() {
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