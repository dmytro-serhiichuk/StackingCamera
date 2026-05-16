package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import android.util.AttributeSet
import android.util.Log
import androidx.appcompat.widget.AppCompatImageView
import androidx.core.view.isVisible
import androidx.exifinterface.media.ExifInterface
import com.sedv.stackingcamera.main.camera.CapturedPhotoInfo
import com.sedv.stackingcamera.main.camera.settings.CameraOutputFormat
import com.sedv.stackingcamera.main.generalsettings.FrameSize
import com.sedv.stackingcamera.main.generalsettings.GeneralSettings
import java.io.ByteArrayInputStream
import java.io.InputStream

class GhostImageView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : AppCompatImageView(context, attrs, defStyleAttr) {
    private var desiredWidth: Int = 0
    private var desiredHeight: Int = 0

    private var ghostBitmap: GhostBitmap? = null

    fun extractBitmap(info: CapturedPhotoInfo): GhostBitmap? {
        if (GeneralSettings.frameSize.value == FrameSize.FRAME_SIZE_16_9.value &&
            info.format == CameraOutputFormat.RAW) return null
        val bitmap = loadScaledBitmap(info) ?: return null
        return GhostBitmap(bitmap, GeneralSettings.frameSize.value)
    }

    fun setBitmap(ghostBitmap: GhostBitmap?) {
        if (ghostBitmap == null || ghostBitmap.frameOption != GeneralSettings.frameSize.value) return
        this.ghostBitmap = ghostBitmap
        setImageBitmap(ghostBitmap.bitmap)
        if (GeneralSettings.ghostImage.value) isVisible = true
    }

    fun setSize(width: Int, height: Int) {
        isVisible = false
        desiredWidth = width
        desiredHeight = height
        requestLayout()
        invalidate()
    }

    fun handleGeneralSettingsChanged() {
        ghostBitmap?.let {
            if (it.frameOption == GeneralSettings.frameSize.value) {
                isVisible = GeneralSettings.ghostImage.value
            }
        }
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)
        if (desiredWidth == 0 || desiredHeight == 0) {
            setMeasuredDimension(width, height)
        } else {
            setMeasuredDimension(desiredWidth, desiredHeight)
        }
    }

    private fun loadScaledBitmap(info: CapturedPhotoInfo): Bitmap? {
        return try {
            val stream = ByteArrayInputStream(info.buffer)

            val willRotate = info.orientation == 90 || info.orientation == 270
            val reqWidth = if (willRotate) desiredHeight else desiredWidth
            val reqHeight = if (willRotate) desiredWidth else desiredHeight

            val options = BitmapFactory.Options().apply { inJustDecodeBounds = true }
            stream.mark(info.buffer.size)
            BitmapFactory.decodeStream(stream, null, options)

            options.inSampleSize = calculateInSampleSize(options, reqWidth, reqHeight)
            options.inJustDecodeBounds = false

            stream.reset()
            val bitmap = BitmapFactory.decodeStream(stream, null, options)
                ?: return null

            rotateBitmapToPortrait(bitmap, info)
        } catch (e: Exception) {
            Log.e("Camera", "Failed to load ghost bitmap: ${e.message}")
            null
        }
    }

    private fun calculateInSampleSize(options: BitmapFactory.Options, reqWidth: Int, reqHeight: Int): Int {
        val (height, width) = options.outHeight to options.outWidth
        var inSampleSize = 1
        while (height / inSampleSize > reqHeight || width / inSampleSize > reqWidth) {
            inSampleSize *= 2
        }
        return inSampleSize
    }

    private fun rotateBitmapToPortrait(bitmap: Bitmap, info: CapturedPhotoInfo): Bitmap {
        return if (info.format == CameraOutputFormat.JPEG) {
            rotateJpegToPortrait(bitmap, info.orientation)
        } else {
            rotateRawToPortrait(bitmap, info.orientation)
        }
    }

    private fun rotateJpegToPortrait(bitmap: Bitmap, orientation: Int): Bitmap {
        val isCurrentlyPortrait = bitmap.height > bitmap.width
        val rotation = when {
            isCurrentlyPortrait && orientation == 180 -> 180f
            isCurrentlyPortrait && orientation == 270 -> 180f
            isCurrentlyPortrait                       -> 0f
            orientation == 180                        -> 270f
            else                                      -> 90f
        }
        return applyRotation(bitmap, rotation)
    }

    private fun rotateRawToPortrait(bitmap: Bitmap, orientation: Int): Bitmap {
        val rotated = applyRotation(bitmap, orientation.toFloat())
        return if (rotated.width > rotated.height) {
            val extraRotation = if (orientation == 180) 270f else 90f
            applyRotation(rotated, extraRotation)
        } else {
            rotated
        }
    }

    private fun applyRotation(bitmap: Bitmap, degrees: Float): Bitmap {
        if (degrees == 0f) return bitmap
        val matrix = Matrix().apply { postRotate(degrees) }
        return Bitmap.createBitmap(bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true)
            .also { bitmap.recycle() }
    }
}

class GhostBitmap(
    val bitmap: Bitmap,
    val frameOption: Int
)