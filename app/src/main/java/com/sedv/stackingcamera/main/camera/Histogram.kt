package com.sedv.stackingcamera.main.camera

import android.media.Image

class Histogram {
    var onUpdated: ((IntArray, IntArray, IntArray, Int) -> Unit)? = null

    private val red   = IntArray(256)
    private val green = IntArray(256)
    private val blue  = IntArray(256)

    private var maximum: Int = 0

    fun update(image: Image) {
        red.fill(0); green.fill(0); blue.fill(0);

        maximum = 0

        val width = image.width
        val height = image.height

        val yPlane = image.planes[0]
        val uPlane = image.planes[1]
        val vPlane = image.planes[2]

        val yBuffer = yPlane.buffer
        val uBuffer = uPlane.buffer
        val vBuffer = vPlane.buffer

        val yRowStride = yPlane.rowStride
        val yPixelStride = yPlane.pixelStride
        val uvRowStride = uPlane.rowStride
        val uvPixelStride = uPlane.pixelStride

        for (row in 0 until height) {
            for (col in 0 until width) {
                val yIndex = row * yRowStride + col * yPixelStride
                val uvRow = row / 2
                val uvCol = col / 2
                val uvIndex = uvRow * uvRowStride + uvCol * uvPixelStride

                val Y = (yBuffer.get(yIndex).toInt() and 0xFF)
                val U = (uBuffer.get(uvIndex).toInt() and 0xFF) - 128
                val V = (vBuffer.get(uvIndex).toInt() and 0xFF) - 128

                val r = (Y + 1.402f * V).toInt()
                val g = (Y - 0.344f * U - 0.714f * V).toInt()
                val b = (Y + 1.772f * U).toInt()

                red[r.coerceIn(0, 255)]++
                green[g.coerceIn(0, 255)]++
                blue[b.coerceIn(0, 255)]++
            }
        }

        maximum = maxOf(red.maxOrNull() ?: 0, green.maxOrNull() ?: 0, blue.maxOrNull() ?: 0)

        onUpdated?.invoke(red, green, blue, maximum)
    }
}