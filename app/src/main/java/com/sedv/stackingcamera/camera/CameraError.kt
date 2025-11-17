package com.sedv.stackingcamera.camera

sealed class CameraError(message: String) : RuntimeException(message) {
    class NotSupported(message: String) : CameraError(message)
    class NotFound(message: String) : CameraError(message)
    class PhotoCreatingFailed(message: String) : CameraError(message)
    class SessionError(message: String) : CameraError(message)
    class OpeningError(message: String): CameraError(message)
    class ClosingError(message: String): CameraError(message)
}