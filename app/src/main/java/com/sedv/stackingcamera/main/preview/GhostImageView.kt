package com.sedv.stackingcamera.main.preview

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Matrix
import android.net.Uri
import android.util.AttributeSet
import android.util.Log
import androidx.appcompat.widget.AppCompatImageView
import androidx.core.view.isVisible
import androidx.lifecycle.findViewTreeLifecycleOwner
import androidx.lifecycle.lifecycleScope
import com.sedv.stackingcamera.main.camera.settings.CameraOutputFormat
import com.sedv.stackingcamera.main.generalsettings.FrameSize
import com.sedv.stackingcamera.main.generalsettings.GeneralSettings
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class GhostImageView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : AppCompatImageView(context, attrs, defStyleAttr) {

    private var loadJob: Job? = null
    private var desiredWidth: Int = 0
    private var desiredHeight: Int = 0

    private var hasLoadedBitmap = false

    internal var ioDispatcher: CoroutineDispatcher = Dispatchers.IO
    internal var mainDispatcher: CoroutineDispatcher = Dispatchers.Main

    fun setImage(info: GhostImageInfo) {
        if (GeneralSettings.frameSize.value == FrameSize.FRAME_SIZE_16_9.value && info.format == CameraOutputFormat.RAW) return
        val scope = findViewTreeLifecycleOwner()?.lifecycleScope ?: return
        loadJob?.cancel()
        loadJob = scope.launch(ioDispatcher) {
            val bitmap = loadScaledBitmap(info) ?: return@launch
            withContext(mainDispatcher) {
                setImageBitmap(bitmap)
                hasLoadedBitmap = true
                if (GeneralSettings.ghostImage.value) isVisible = true
            }
        }
    }

    fun clear() {
        loadJob?.cancel()
        isVisible = false
    }

    fun setSize(width: Int, height: Int) {
        clear()
        desiredWidth = width
        desiredHeight = height
        requestLayout()
        invalidate()
    }

    fun handleGeneralSettingsChanged() {
        if (hasLoadedBitmap) {
            isVisible = !isVisible
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

    private fun loadScaledBitmap(info: GhostImageInfo): Bitmap? {
        return try {
            val options = BitmapFactory.Options().apply { inJustDecodeBounds = true }
            val contentResolver = context.contentResolver
            contentResolver.openInputStream(info.uri)?.use {
                BitmapFactory.decodeStream(it, null, options)
            }

            val willRotate = info.orientation == 90 || info.orientation == 270
            val reqWidth = if (willRotate) desiredHeight else desiredWidth
            val reqHeight = if (willRotate) desiredWidth else desiredHeight
            options.inSampleSize = calculateInSampleSize(options, reqWidth, reqHeight)
            options.inJustDecodeBounds = false

            val bitmap = contentResolver.openInputStream(info.uri)?.use {
                BitmapFactory.decodeStream(it, null, options)
            } ?: return null

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

    private fun rotateBitmapToPortrait(bitmap: Bitmap, info: GhostImageInfo): Bitmap {
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

class GhostImageInfo(
    val uri: Uri,
    val orientation: Int,
    val format: CameraOutputFormat
)