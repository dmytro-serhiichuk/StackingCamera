package com.sedv.stackingcamera

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
import com.sedv.stackingcamera.camera.CameraError
import com.sedv.stackingcamera.camera.PhotoType
import com.sedv.stackingcamera.camera.settings.CameraOutputFormat
import com.sedv.stackingcamera.databinding.ActivityMainBinding
import com.sedv.stackingcamera.settings.BaseProperty
import com.sedv.stackingcamera.settings.FrameSize
import com.sedv.stackingcamera.settings.GeneralPropertyType
import com.sedv.stackingcamera.settings.GeneralSettings
import com.sedv.stackingcamera.stacking.StackingActivity
import com.sedv.stackingcamera.ui.GeneralSettingsController
import com.sedv.stackingcamera.ui.PreviewBottomContainerManager
import com.sedv.stackingcamera.ui.ShutterController
import com.sedv.stackingcamera.ui.preview.GhostImageInfo
import com.sedv.stackingcamera.ui.preview.GhostImageView
import com.sedv.stackingcamera.ui.preview.Preview
import com.sedv.stackingcamera.ui.settings.SettingsController
import com.sedv.stackingcamera.ui.switcher.Switcher
import com.sedv.stackingcamera.viewmodels.AppViewModel
import com.sedv.stackingcamera.viewmodels.CameraViewModel
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import java.io.File
import kotlin.system.exitProcess

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding

    private val viewModel: AppViewModel by viewModels()
    private lateinit var permissionHelper: PermissionHelper
    private var isSuccessfullyInitialized = false
    private var hasReturnedFromSettings = false

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

        permissionHelper = PermissionHelper(this)
        GeneralSettings.onChanged += ::handleGeneralSettingsChanged

        if (permissionHelper.hasAllPermissions()) {
            initializeApp()
        } else {
            permissionHelper.requestAllPermissions()
        }
    }

    private fun initializeApp() {
        try {
            viewModel.init(permissionHelper)
            viewModel.cameraViewModel.onCameraSwitched += ::handleCameraSwitched
            viewModel.cameraViewModel.onProgramReady += ::startCameraFlow

            // Shutter Button
            shutterController = ShutterController(
                this,
                viewModel,
                binding.shutterButton,
                binding.shutterButtonAdditional,
                binding.screenIndicator
            )

            previewBottomContainerManager = PreviewBottomContainerManager(binding.onPreviewBottomContainer)

            // Camera Switcher
            cameraSwitcher = Switcher(
                this,
                viewModel.cameraViewModel,
                binding.cameraSwitcher,
                binding.cameraList,
                previewBottomContainerManager
            )

            // Camera Settings
            settingsController = SettingsController(
                this,
                viewModel.cameraViewModel,
                previewBottomContainerManager,
                binding.settingsList,
                binding.sliderViewWrapper,
                binding.autoModeToggle
            )

            // General Settings
            generalSettingsController = GeneralSettingsController(
                this,
                viewModel.cameraViewModel,
                binding.generalSettings,
                binding.generalSettingsBottomContainer
            )

            // Preview
            preview = Preview(
                this,
                viewModel.cameraViewModel,
                binding.surfaceContainer,
                binding.histogramView,
                binding.meteringArea,
                binding.zoomIndicator,
                binding.gridView,
                binding.screenIndicator,
                binding.ghostImageView
            )
            viewModel.cameraViewModel.ghostImageInfo?.let { info ->
                binding.ghostImageView.setImage(info)
            }

//            TODO: add another button for photo passing
            binding.navToStackingButton.setOnClickListener {
                val intent = Intent(this, StackingActivity::class.java)
                if (viewModel.cameraViewModel.lastBurstPhotosUris.isNotEmpty()) {
                    intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, ArrayList(viewModel.cameraViewModel.lastBurstPhotosUris))
                    viewModel.cameraViewModel.lastBurstPhotosUris.clear()
                }
                startActivity(intent)
            }

            val orientationListener = object : OrientationEventListener(this) {
                override fun onOrientationChanged(orientation: Int) {
                    val oldDeviceOrientation = viewModel.cameraViewModel.deviceOrientation
                    viewModel.cameraViewModel.updateOrientation(orientation)
                    if (viewModel.cameraViewModel.deviceOrientation != oldDeviceOrientation) {
                        binding.histogramView.rotate(viewModel.cameraViewModel.deviceOrientation)
                    }
                }
            }

            orientationListener.enable()

            isSuccessfullyInitialized = true
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

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        permissionHelper.onRequestPermissionsResult(requestCode, permissions, grantResults) { granted ->
            if (granted && !isSuccessfullyInitialized) {
                initializeApp()
            }
        }
    }

    fun onReturnFromSettings() {
        hasReturnedFromSettings = true
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
                .setPositiveButton("OK") { dialog, id ->
                    finishAffinity()
                    onClosed()
                }
            val dialog = builder.create()
            dialog.show()
        }
    }

    private fun startCameraFlow() {
        val surface = viewModel.cameraViewModel.previewSurface

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
            viewModel.cameraViewModel.activeCamera.onPhotoCreated = ::handlePhotoCreated
            viewModel.cameraViewModel.activeCamera.onPhotoCreatingFailed = { e ->
                Toast.makeText(this, e.message, Toast.LENGTH_SHORT).show()
            }

            viewModel.cameraViewModel.activeCamera.onPhotoReceived += {
                val color = if (viewModel.cameraViewModel.activeCamera.captureAnalyser.meanBrightness > 64.0) {
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
            val hasPermissions = permissionHelper.hasAllPermissions()

            if (hasPermissions && isSuccessfullyInitialized) {
                viewModel.cameraViewModel.resume()
            } else if (hasPermissions && !isSuccessfullyInitialized) {
                initializeApp()
            } else if (!hasPermissions && hasReturnedFromSettings) {
                permissionHelper.requestAllPermissions()
                hasReturnedFromSettings = false
            }
        } catch (e: CameraError) {
            Toast.makeText(this, e.message, Toast.LENGTH_SHORT).show()
        }
    }
    override fun onPause() {
        super.onPause()

        try {
            viewModel.cameraViewModel.pause()
        } catch (e: CameraError) {
            Toast.makeText(this, e.message, Toast.LENGTH_SHORT).show()
        }
    }
    override fun onDestroy() {
        super.onDestroy()

        viewModel.cameraViewModel.destroy()

        Log.d("MainActivity", "Activity destroyed, resources cleaned up")
    }

    private fun handlePhotoCreated(bytes: ByteArray, format: CameraOutputFormat, photoType: PhotoType, orientation: Int) {
        val timestamp = System.currentTimeMillis()
        val filename = "IMG_${timestamp}${format.ext}"

        if (photoType == PhotoType.BURST_FIRST) {
            viewModel.cameraViewModel.lastBurstPhotosUris.clear()
        }

        try {
            val ghostImageUri: Uri? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                // Android 10+ (API 29+)
                val contentValues = ContentValues().apply {
                    put(MediaStore.MediaColumns.DISPLAY_NAME, filename)
                    put(MediaStore.MediaColumns.MIME_TYPE, format.mimeType)
                    put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_DCIM + "/Camera")
                }

                val resolver = contentResolver
                val imageUri = resolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues)

                imageUri?.also { uri ->
                    resolver.openOutputStream(uri)?.use { outputStream ->
                        outputStream.write(bytes)
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
                file.writeBytes(bytes)

                MediaScannerConnection.scanFile(
                    this,
                    arrayOf(file.absolutePath),
                    arrayOf(format.mimeType),
                    null
                )
                Log.d("Camera", "Image saved to: ${file.absolutePath}")
                Uri.fromFile(file)
            }

            ghostImageUri?.let { uri ->
                if (photoType != PhotoType.REGULAR) {
                    viewModel.cameraViewModel.lastBurstPhotosUris.add(uri)
                }
                if (photoType == PhotoType.REGULAR || photoType == PhotoType.BURST_LAST) {
                    viewModel.cameraViewModel.ghostImageInfo = GhostImageInfo(uri, orientation, format)
                    binding.ghostImageView.setImage(viewModel.cameraViewModel.ghostImageInfo!!)
                }
            }
        } catch (e: Exception) {
            Log.e("Camera", "Failed to save image: ${e.message}")
        }
    }
}