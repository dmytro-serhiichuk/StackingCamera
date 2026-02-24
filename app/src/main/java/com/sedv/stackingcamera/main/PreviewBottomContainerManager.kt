package com.sedv.stackingcamera.main

import android.view.ViewGroup
import android.widget.FrameLayout
import androidx.core.view.isVisible

class PreviewBottomContainerManager(
    private val container: FrameLayout,
) {
    var onCleared: (() -> Unit)? = null

    fun showGroup(group: ViewGroup) {
        container.isVisible = true
        container.removeAllViews()
        container.addView(group)
    }

    fun clear() {
        container.removeAllViews()
        container.isVisible = false
        onCleared?.invoke()
    }
}