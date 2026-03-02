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
        binding.bitmapName.text = "${bitmapInfo.name}"
        updateAdditionalInfoText()
        updateReferenceFrameLabel()
        updateWarningMessage()

        binding.removeButton.setOnClickListener {
            onRemoveCallback(this)
        }
    }

    fun updateAdditionalInfoText() {
        if (bitmapInfo.score != -1) {
            binding.bitmapAdditionalInfo.text = "${bitmapInfo.width} x ${bitmapInfo.height} | Keypoints: ${bitmapInfo.score}"
        } else {
            binding.bitmapAdditionalInfo.text = "${bitmapInfo.width} x ${bitmapInfo.height}"
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

                val level = if (hasBadStatus) WarningLevel.BAD else WarningLevel.WARNING
                binding.bitmapWarningMessage.setValues(level, vInfo.getMessage())
            } else {
                binding.bitmapWarningMessage.isVisible = false
            }
        }
    }

    fun setEnableMode(mode: Boolean) {
        binding.removeButton.isEnabled = mode
    }
}