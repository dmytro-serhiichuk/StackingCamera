package com.sedv.stackingcamera.main.camera

import com.sedv.stackingcamera.main.camera.settings.CameraOutputFormat

class CapturedPhotoInfo(
    val buffer: ByteArray,
    val format: CameraOutputFormat,
    val photoType: PhotoType,
    val orientation: Int
)