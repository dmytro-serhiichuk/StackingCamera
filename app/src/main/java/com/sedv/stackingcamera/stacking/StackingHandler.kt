package com.sedv.stackingcamera.stacking

object StackingHandler {
    val bitmaps = arrayListOf<BitmapInfo>()
    var referenceBitmap: BitmapInfo? = null
    var hasMatches = false
    var hasStackedResult = false

    var state = StackingState.NOT_READY

    fun hasEnoughBitmaps(): Boolean {
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