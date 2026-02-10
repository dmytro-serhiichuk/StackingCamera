package com.sedv.stackingcamera.ui.switcher

import android.content.Context
import android.view.LayoutInflater
import android.view.View.GONE
import android.view.View.VISIBLE
import android.widget.HorizontalScrollView
import android.widget.LinearLayout
import android.widget.Toast
import androidx.appcompat.widget.AppCompatButton
import androidx.appcompat.widget.AppCompatImageButton
import androidx.appcompat.widget.AppCompatTextView
import androidx.core.view.isVisible
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.sedv.stackingcamera.viewmodels.CameraViewModel
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.camera.CameraError
import com.sedv.stackingcamera.camera.CameraType
import com.sedv.stackingcamera.ui.PreviewBottomContainerManager

class Switcher(
    context: Context,
    private val viewModel: CameraViewModel,
    private val switcherButton: AppCompatImageButton,
    private val cameraListContainer: LinearLayout,
    private val previewBottomContainer: PreviewBottomContainerManager
) {
    private var isFacingFront = false
    private var lastBackCameraId = viewModel.activeCamera.cameraInfo.cameraId

    private lateinit var selectedCamera: AppCompatButton

    init {
        val frontCamera = viewModel.cameraController.cameras.values.find { it.cameraInfo.isFront }

        if (frontCamera == null) {
            switcherButton.isVisible = false
        } else {
            switcherButton.setOnClickListener {
                val oldValue = isFacingFront
                isFacingFront = !isFacingFront
                if (oldValue) {
                    previewBottomContainer.showGroup(cameraListContainer)
                    viewModel.selectCamera(lastBackCameraId)
                } else {
                    previewBottomContainer.clear()
                    viewModel.selectCamera(frontCamera.cameraInfo.cameraId)
                }
            }
        }

        for (camera in this.viewModel.cameraController.cameras.values) {
            val info = camera.cameraInfo
            if (info.cameraType == CameraType.LOGICAL || info.isFront) continue

            val view = LayoutInflater.from(context).inflate(R.layout.camera_list_item, cameraListContainer, false) as AppCompatButton
            view.text = info.cameraLabel
            view.setOnClickListener {
                try {
                    selectedCamera.isSelected = false
                    viewModel.selectCamera(info.cameraId)
                    view.isSelected = true
                    selectedCamera = view
                    lastBackCameraId = info.cameraId
                } catch (e: CameraError) {
                    Toast.makeText(context, e.message, Toast.LENGTH_SHORT).show()
                }
            }
            cameraListContainer.addView(view)

            if (camera == viewModel.activeCamera) {
                selectedCamera = view
                view.isSelected = true
            }
        }

        previewBottomContainer.onCleared = {
            if (!isFacingFront) {
                previewBottomContainer.showGroup(cameraListContainer)
            }
        }
    }
}