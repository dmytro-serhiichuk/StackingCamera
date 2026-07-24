package com.sedv.stackingcamera.main

import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.ContentValues
import android.content.Intent
import android.graphics.Color
import android.media.MediaScannerConnection
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.MediaStore
import android.util.Log
import android.view.MotionEvent
import android.view.OrientationEventListener
import android.view.View
import android.view.WindowManager
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.WindowCompat
import androidx.lifecycle.lifecycleScope
import com.sedv.stackingcamera.R
import com.sedv.stackingcamera.main.camera.CameraError
import com.sedv.stackingcamera.main.camera.PhotoType
import com.sedv.stackingcamera.databinding.ActivityMainBinding
import com.sedv.stackingcamera.main.camera.CapturedPhotoInfo
import com.sedv.stackingcamera.main.generalsettings.property.BaseProperty
import com.sedv.stackingcamera.main.generalsettings.FrameSize
import com.sedv.stackingcamera.main.generalsettings.GeneralPropertyType
import com.sedv.stackingcamera.main.generalsettings.GeneralSettings
import com.sedv.stackingcamera.stacking.StackingActivity
import com.sedv.stackingcamera.main.generalsettings.GeneralSettingsController
import com.sedv.stackingcamera.main.preview.Preview
import com.sedv.stackingcamera.main.settings.SettingsController
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import kotlin.system.exitProcess

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    private val viewModel: CameraViewModel by viewModels()
    private var isViewsReady = false

    private lateinit var preview: Preview
    private lateinit var shutterController: ShutterController
    private lateinit var cameraSwitcher: Switcher
    private lateinit var settingsController: SettingsController
    private lateinit var generalSettingsController: GeneralSettingsController
    private lateinit var previewBottomContainerManager: PreviewBottomContainerManager


    @SuppressLint("ClickableViewAccessibility")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        WindowCompat.setDecorFitsSystemWindows(window, true)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        viewModel.init(this)
        GeneralSettings.onChanged += ::handleGeneralSettingsChanged

        if (viewModel.permissionHelper.hasAllPermissions()) {
            initViews()
        } else {
            viewModel.permissionHelper.requestAllPermissions()
        }
    }

    private fun initViews() {
        try {
            viewModel.onCameraSwitched += ::handleCameraSwitched
            viewModel.onProgramReady += ::startCameraFlow

            // Shutter Button
            shutterController = ShutterController(
                this,
                viewModel,
                binding.shutterButton,
                binding.shutterButtonAdditional,
                binding.screenIndicator
            )

            previewBottomContainerManager =
                PreviewBottomContainerManager(binding.onPreviewBottomContainer)

            // Camera Switcher
            cameraSwitcher = Switcher(
                this,
                viewModel,
                binding.cameraSwitcher,
                binding.cameraList,
                previewBottomContainerManager
            )

            // Camera Settings
            settingsController = SettingsController(
                this,
                viewModel,
                previewBottomContainerManager,
                binding.settingsList,
                binding.sliderViewWrapper,
                binding.autoModeToggle
            )

            // General Settings
            generalSettingsController = GeneralSettingsController(
                this,
                viewModel,
                binding.generalSettings,
                binding.generalSettingsBottomContainer
            )

            // Preview
            preview = Preview(
                this,
                viewModel,
                binding.surfaceContainer,
                binding.histogramView,
                binding.meteringArea,
                binding.evCorrectionSlider,
                binding.zoomIndicator,
                binding.gridView,
                binding.screenIndicator,
                binding.ghostImageView
            )
            binding.ghostImageView.setBitmap(viewModel.ghostBitmap)

//            TODO: add another button for photo passing
            binding.navToStackingButton.setOnClickListener {
                val intent = Intent(this, StackingActivity::class.java)
                if (viewModel.lastBurstPhotosUris.isNotEmpty()) {
                    intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, ArrayList(viewModel.lastBurstPhotosUris))
                    viewModel.lastBurstPhotosUris.clear()
                }
                startActivity(intent)
            }

            val orientationListener = object : OrientationEventListener(this) {
                override fun onOrientationChanged(orientation: Int) {
                    val oldDeviceOrientation = viewModel.deviceOrientation
                    viewModel.updateOrientation(orientation)
                    if (viewModel.deviceOrientation != oldDeviceOrientation) {
                        binding.histogramView.rotate(viewModel.deviceOrientation)
                    }
                }
            }

            orientationListener.enable()

            isViewsReady = true
        }
//    catch (e: CameraError) {
//        showAlert(
//            "Fatal Error",
//            e.message,
//            { exitProcess(-1) }
//        )
//    }
        catch (e: Exception) {
            showAlert(
                "Fatal Error",
                e.stackTraceToString(),
                { exitProcess(-1) }
            )
        }
    }

    override fun dispatchTouchEvent(ev: MotionEvent?): Boolean {
        ev?.let { event ->
            if (event.action == MotionEvent.ACTION_DOWN) {
                val x = event.rawX
                val y = event.rawY

                if (settingsController.isOpen &&
                    !isPointInsideView(binding.onPreviewBottomContainer, x, y) &&
                    !isPointInsideView(binding.settingsListWrapper, x, y) &&
                    isPointInsideView(binding.surfaceReferenceFrame, x, y)) {
                    settingsController.closeAdditionalControls()
                }
                if (generalSettingsController.isOpen &&
                    !isPointInsideView(binding.generalSettings, x, y) &&
                    !isPointInsideView(binding.generalSettingsBottomContainer, x, y)) {
                    generalSettingsController.close()
                }
            }
        }
        return super.dispatchTouchEvent(ev)
    }

    private fun isPointInsideView(view: View, x: Float, y: Float): Boolean {
        val location = IntArray(2)
        view.getLocationOnScreen(location)
        val viewX = location[0]
        val viewY = location[1]

        return (x >= viewX && x <= (viewX + view.width) &&
                y >= viewY && y <= (viewY + view.height))
    }

    private fun showAlert(title: String, message: String?, onClosed: () -> Unit) {
        runOnUiThread {
            val builder = AlertDialog.Builder(this)
                .setTitle(title)
                .setMessage(message ?: "Unhandled error")
                .setCancelable(false)
                .setPositiveButton("OK") { _, _ ->
                    finishAffinity()
                    onClosed()
                }
            val dialog = builder.create()
            dialog.show()
        }
    }

    private fun startCameraFlow() {
        val surface = viewModel.previewSurface

        if (surface == null || !surface.isValid) {
            Log.e("MainActivity", "Preview surface is null or invalid")
            return
        }

        try {
            Log.d("MainActivity", "Starting camera flow")

            handleCameraSwitched()

        } catch (e: Exception) {
            Log.e("MainActivity", "Error starting camera: ${e.message}", e)
            Toast.makeText(this, "Error starting camera: ${e.message}", Toast.LENGTH_LONG).show()
        }
    }

    private fun handleCameraSwitched() {
        try {
            viewModel.activeCamera.onPhotoCreated = ::handlePhotoCreated
            viewModel.activeCamera.onPhotoCreatingFailed = { e ->
                Toast.makeText(this, e.message, Toast.LENGTH_SHORT).show()
            }

            viewModel.activeCamera.onPhotoReceived += {
                val color = if (viewModel.activeCamera.captureAnalyser.meanBrightness > 64.0) {
                    Color.BLACK
                } else {
                    Color.WHITE
                }
                binding.screenIndicator.setBackgroundColor(color)
                lifecycleScope.launch {
                    delay(50)
                    binding.screenIndicator.setBackgroundColor(Color.TRANSPARENT)
                }
            }

        } catch (e: CameraError) {
            Toast.makeText(this, e.message, Toast.LENGTH_SHORT).show()
        }
    }

    private fun handleGeneralSettingsChanged(prop: BaseProperty<*>) {
        if (prop.type == GeneralPropertyType.FRAME_SIZE) {
            binding.surfaceReferenceFrame.handleFrameSizeChanged()
            val frameSize = GeneralSettings.frameSize.value
            if (frameSize == FrameSize.FRAME_SIZE_4_3.value) {
                binding.settingsListWrapper.setBackgroundResource(R.drawable.round_block_background)
            } else {
                binding.settingsListWrapper.setBackgroundResource(R.drawable.round_block_background_20)
            }
        }
    }

    override fun onResume() {
        super.onResume()

        try {
            val hasPermissions = viewModel.permissionHelper.hasAllPermissions()

            if (hasPermissions && isViewsReady) {
                viewModel.resume()
            } else if (hasPermissions) {
                initViews()
            } else {
                viewModel.permissionHelper.requestAllPermissions()
            }
        } catch (e: CameraError) {
            Toast.makeText(this, e.message, Toast.LENGTH_SHORT).show()
        }
    }
    override fun onPause() {
        super.onPause()

        try {
            viewModel.pause()
        } catch (e: CameraError) {
            Toast.makeText(this, e.message, Toast.LENGTH_SHORT).show()
        }
    }
    override fun onDestroy() {
        super.onDestroy()

        preview.release()
        viewModel.destroy()

        Log.d("MainActivity", "Activity destroyed, resources cleaned up")
    }

    private fun handlePhotoCreated(info: CapturedPhotoInfo) {
        val timestamp = System.currentTimeMillis()
        val filename = "IMG_${timestamp}${info.format.ext}"

        if (info.photoType == PhotoType.BURST_FIRST) {
            viewModel.lastBurstPhotosUris.clear()
        }

        lifecycleScope.launch(Dispatchers.Default) {
            viewModel.ghostBitmap = binding.ghostImageView.extractBitmap(info)
            withContext(Dispatchers.Main) {
                binding.ghostImageView.setBitmap(viewModel.ghostBitmap)
            }
        }

        try {
            val lastImageUri: Uri? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                val contentValues = ContentValues().apply {
                    put(MediaStore.MediaColumns.DISPLAY_NAME, filename)
                    put(MediaStore.MediaColumns.MIME_TYPE, info.format.mimeType)
                    put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DCIM + "/Camera")
                }

                val resolver = contentResolver
                val imageUri = resolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues)

                imageUri?.also { uri ->
                    resolver.openOutputStream(uri)?.use { outputStream ->
                        outputStream.write(info.buffer)
                        outputStream.flush()
                    }
                    Log.d("Camera", "Image saved to MediaStore: $filename")
                }
            } else {
                val picturesDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DCIM)
                val cameraDir = File(picturesDir, "Camera")
                if (!cameraDir.exists()) {
                    cameraDir.mkdirs()
                }

                val file = File(cameraDir, filename)
                file.writeBytes(info.buffer)

                MediaScannerConnection.scanFile(
                    this,
                    arrayOf(file.absolutePath),
                    arrayOf(info.format.mimeType),
                    null
                )
                Log.d("Camera", "Image saved to: ${file.absolutePath}")
                Uri.fromFile(file)
            }

            lastImageUri?.let { uri ->
                if (info.photoType != PhotoType.REGULAR) {
                    viewModel.lastBurstPhotosUris.add(uri)
                }
            }
        } catch (e: Exception) {
            Log.e("Camera", "Failed to save image: ${e.message}")
        }
    }
}