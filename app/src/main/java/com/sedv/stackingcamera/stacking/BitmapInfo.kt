package com.sedv.stackingcamera.stacking

class BitmapInfo(
    val width: Int,
    val height: Int,
    var name: String = "null",
    var score: Int = -1
) {
    val isAnalyzed: Boolean get() = score > -1
}