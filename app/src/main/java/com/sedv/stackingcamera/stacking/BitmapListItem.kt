package com.sedv.stackingcamera.stacking

import android.content.Context
import android.view.LayoutInflater
import android.widget.LinearLayout
import androidx.core.content.ContextCompat
import androidx.core.view.isVisible
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.databinding.BitmapListItemBinding

class BitmapListItem(
    context: Context,
    val bitmapInfo: BitmapInfo,
    val onRemoveCallback: (BitmapListItem) -> Unit
) : LinearLayout(context) {

    private val binding = BitmapListItemBinding.inflate(LayoutInflater.from(context), this, true)

    init {
        binding.bitmapName.text   = "Name: ${bitmapInfo.name}"
        binding.bitmapWidth.text  = "Width: ${bitmapInfo.width}"
        binding.bitmapHeight.text = "Height: ${bitmapInfo.height}"
        updateScore()
        updateReferenceFrameLabel()
        updateWarningMessage()

        binding.removeButton.setOnClickListener {
            onRemoveCallback(this)
        }
    }

    fun updateScore() {
        if (bitmapInfo.score != -1) {
            binding.bitmapScore.visibility = VISIBLE
            binding.bitmapScore.text  = "Score: ${bitmapInfo.score}"
        } else {
            binding.bitmapScore.visibility = INVISIBLE
        }
    }

    fun updateReferenceFrameLabel() {
        binding.referenceFrameLabel.isVisible = bitmapInfo.isReferenceFrame
    }

    fun updateWarningMessage() {
        if (bitmapInfo.homographyValidationInfo == null) {
            binding.bitmapWarningMessage.isVisible = false
        } else {
            val vInfo = bitmapInfo.homographyValidationInfo!!
            val hasWarningStatus = vInfo.props.any { it == HomographyValidationStatus.WARNING }
            val hasBadStatus = vInfo.props.any { it == HomographyValidationStatus.BAD } || !vInfo.isConvex || vInfo.mirrored

            if (hasBadStatus || hasWarningStatus) {
                binding.bitmapWarningMessage.isVisible = true

                binding.bitmapWarningMessage.setTextColor(
                    if (hasBadStatus) ContextCompat.getColor(context, R.color.error_text_color)
                    else ContextCompat.getColor(context, R.color.warning_text_color)
                )
            } else {
                binding.bitmapWarningMessage.isVisible = false
            }
        }
    }

    fun setEnableMode(mode: Boolean) {
        binding.removeButton.isEnabled = mode
    }
}