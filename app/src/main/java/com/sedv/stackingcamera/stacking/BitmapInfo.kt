package com.sedv.stackingcamera.stacking

class BitmapInfo {
    val width: Int
    val height: Int
    var name: String
    var score: Int
    val isInitialized: Boolean get() = score > -1

    constructor(w: Int, h: Int) {
        width = w
        height = h
        score = -1
        name = "null"
    }

    constructor() {
        width = -1
        height = -1
        score = -1
        name = "null"
    }
}