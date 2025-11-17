package com.sedv.stackingcamera.ui.preview

import android.content.Context
import android.util.AttributeSet
import android.widget.FrameLayout
import com.sedv.stackingcamera.settings.FrameSize
import com.sedv.stackingcamera.settings.GeneralSettings

class PreviewReferenceFrame @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : FrameLayout(context, attrs, defStyleAttr) {

    private var heightRatio = 4f / 3f

    init {
        clipChildren = false
        clipToPadding = false
    }

    fun handleFrameSizeChanged() {
        val frameSize = GeneralSettings.frameSize.value
        heightRatio = if (frameSize == FrameSize.FRAME_SIZE_4_3.value) 4f / 3f
        else 8f / 6.6f
        requestLayout()
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)

        val width = measuredWidth
        val height = width * heightRatio
        setMeasuredDimension(width, height.toInt())
    }
}