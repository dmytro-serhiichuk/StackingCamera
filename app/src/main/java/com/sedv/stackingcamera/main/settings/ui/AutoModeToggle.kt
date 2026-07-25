package com.sedv.stackingcamera.main.settings.ui

import android.content.Context
import android.util.AttributeSet
import androidx.appcompat.widget.AppCompatButton
import androidx.core.view.isVisible

class AutoModeToggle @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet,
    defStyleAttr: Int = 0
) : AppCompatButton(context, attrs, defStyleAttr) {

    var onVisibilityUpdated: ((Boolean) -> Unit)? = null

    var buttonVisibility
        get() = isVisible
        set(value) {
            isVisible = value
            onVisibilityUpdated?.invoke(isVisible)
        }
}