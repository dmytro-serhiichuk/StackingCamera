package com.sedv.stackingcamera.camera

import android.media.ImageReader
import android.os.Handler
import com.sedv.stackingcamera.camera.settings.CameraOutputFormat

class FormatImageReader {
    val format: CameraOutputFormat
    private val imageReader: ImageReader

    val surface get() = imageReader.surface

    constructor(format: CameraOutputFormat, width: Int, height: Int, maxImages: Int) {
        this.format = format

        imageReader = ImageReader.newInstance(
            width,
            height,
            format.value,
            maxImages
        )
    }

    fun setOnImageAvailableListener(listener: ImageReader.OnImageAvailableListener, handler: Handler) {
        imageReader.setOnImageAvailableListener(listener, handler)
    }

    fun close() {
        imageReader.close()
    }
}