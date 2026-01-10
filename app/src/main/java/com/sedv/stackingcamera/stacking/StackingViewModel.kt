package com.sedv.stackingcamera.stacking

import androidx.lifecycle.ViewModel

class StackingViewModel : ViewModel() {
    val bitmaps = arrayListOf<BitmapInfo>()
    var isAlignmentDisabled = false

    fun canStack(): Boolean {
        if (bitmaps.isEmpty() || isAlignmentDisabled) return false
        for (bitmap in bitmaps) {
            if (!bitmap.isInitialized) return false
        }
        return true
    }
}