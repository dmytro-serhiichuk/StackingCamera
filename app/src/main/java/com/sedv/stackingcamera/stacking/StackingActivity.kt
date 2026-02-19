package com.sedv.stackingcamera.stacking

import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.ContentValues
import android.content.Intent
import android.content.res.AssetManager
import android.media.MediaScannerConnection
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.os.ParcelFileDescriptor
import android.provider.MediaStore
import android.provider.OpenableColumns
import android.view.WindowManager
import android.widget.ArrayAdapter
import android.widget.AutoCompleteTextView
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.forEach
import androidx.core.view.forEachIndexed
import androidx.core.view.isVisible
import androidx.lifecycle.lifecycleScope
import com.sedv.stackingcamera.databinding.ActivityStackingBinding
import com.sedv.stackingcamera.stacking.settings.Settings
import com.sedv.stackingcamera.stacking.settings.StackingSettingsActivity
import com.sedv.stackingcamera.viewmodels.AppViewModel
import com.sedv.stackingcamera.viewmodels.StackingState
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import kotlin.system.exitProcess

class StackingActivity : AppCompatActivity() {
    private lateinit var binding: ActivityStackingBinding

    private val viewModel: AppViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityStackingBinding.inflate(layoutInflater)
        setContentView(binding.root)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        if (viewModel.stackingViewModel.state == StackingState.NOT_READY) {
            cacheDir.listFiles()?.forEach { f ->
                f.delete()
            }
            lifecycleScope.launch {
                try {
                    withContext(Dispatchers.IO) {
                        initStacking(applicationContext.assets)
                        Settings.initialize(getSharedPreferences("SETTINGS", MODE_PRIVATE))
                    }
                    viewModel.stackingViewModel.state = StackingState.IDLE
                    updateButtonsState()
                } catch (e: Exception) {
                    showFatalError(e)
                }
            }
        } else {
            for (bitmap in viewModel.stackingViewModel.bitmaps) {
                binding.loadedImagesList.addView(BitmapListItem(
                    this, bitmap, ::handleBitmapRemoved
                ))
            }
            if (viewModel.stackingViewModel.bitmaps.isNotEmpty()) {
                binding.loadedImagesCount.isVisible = true
                binding.loadedImagesCount.text = "Images: ${viewModel.stackingViewModel.bitmaps.size}"
            }
        }

        binding.navToCameraButton.setOnClickListener {
            finish()
        }

        binding.navToStackingSettingsButton.setOnClickListener {
            val intent = Intent(this, StackingSettingsActivity::class.java)
            startActivity(intent)
        }
        updateButtonsState()

        binding.loadImagesBtn.setOnClickListener { handleLoadImagesButtonClicked() }
        binding.analyseBtn.setOnClickListener { handleAnalyseButtonClicked() }
        binding.stackBtn.setOnClickListener { handleStackButtonClicked() }
        binding.saveBtn.setOnClickListener { handleSaveButtonClicked() }

        binding.disableAlignmentCheckBox.setOnCheckedChangeListener { _, isChecked ->
            updateButtonsState()
        }

        val outputFormatAdapter = ArrayAdapter(
            this,
            android.R.layout.simple_list_item_1,
            OUTPUT_FORMATS
        )
        outputFormatAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        (binding.outputFormatSelector.editText as? AutoCompleteTextView)?.setAdapter(outputFormatAdapter)
        binding.outputFormatSelectorAutoCompleteTextView.setText(OUTPUT_FORMATS[0], false)

        binding.logWindow.onCloseButtonClicked = ::handleLogWindowsClosed

        val uris = intent.getParcelableArrayListExtra<Uri>(Intent.EXTRA_STREAM)
        if (!uris.isNullOrEmpty()) {
            startAction()
            loadImages(uris)
        }
    }

    private fun showFatalError(e: Exception) {
        val builder = AlertDialog.Builder(this)
            .setTitle("Fatal Error")
            .setMessage(e.message)
            .setCancelable(false)
            .setPositiveButton("OK") { dialog, id ->
                finishAffinity()
                exitProcess(0)
            }
        val dialog = builder.create()
        dialog.show()
    }

    private fun updateButtonsState() {
        val isIdle = viewModel.stackingViewModel.state == StackingState.IDLE
        binding.loadImagesBtn.isEnabled = isIdle
        binding.analyseBtn.isEnabled = isIdle && viewModel.stackingViewModel.bitmaps.isNotEmpty()
        binding.stackBtn.isEnabled = isIdle && viewModel.stackingViewModel.canStack() && (viewModel.stackingViewModel.isAllBitmapsInitialized() || binding.disableAlignmentCheckBox.isChecked)
        binding.saveBtn.isEnabled = isIdle && viewModel.stackingViewModel.hasStackedResult
        binding.loadedImagesList.forEach {
            val bitmapListItem = it as BitmapListItem
            bitmapListItem.setEnableMode(isIdle)
        }
        if (viewModel.stackingViewModel.state == StackingState.BUSY) {
            binding.logWindow.isVisible = true
            binding.logWindowBackground.isVisible = true
            binding.logWindow.reset()
        }
        else if (isIdle) {
            binding.logWindow.markFinished()
        }
        binding.navToCameraButton.isEnabled = isIdle
        binding.navToStackingSettingsButton.isEnabled = isIdle
    }
    private fun startAction() {
        viewModel.stackingViewModel.state = StackingState.BUSY
        updateButtonsState()
    }
    private fun endAction() {
        viewModel.stackingViewModel.state = StackingState.IDLE
        updateButtonsState()
    }

    private fun handleLoadImagesButtonClicked() {
        startAction()
        val intent = Intent(Intent.ACTION_GET_CONTENT).apply {
            type = "*/*"
            addCategory(Intent.CATEGORY_OPENABLE)
            putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true)
        }
        startActivityForResult(intent, PICK_IMAGE_REQUEST)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == PICK_IMAGE_REQUEST) {
            if (resultCode == RESULT_OK) {
                val uris = extractUrisFromIntent(data)
                uris?.let {
                    loadImages(it)
                }
            }
            if (resultCode == RESULT_CANCELED) {
                endAction()
                handleLogWindowsClosed()
            }
        }
    }

    private fun extractUrisFromIntent(data: Intent?): ArrayList<Uri>? {
        if (data == null || (data.data == null && data.clipData == null)) return null

        val selectedFiles = arrayListOf<Uri>()
        data.clipData?.let { clip ->
            repeat(clip.itemCount) {
                selectedFiles.add(clip.getItemAt(it).uri)
            }
        } ?: data.data?.let {
            selectedFiles.add(it)
        }

        return selectedFiles
    }

    private fun loadImages(uris: ArrayList<Uri>) {
        lifecycleScope.launch {
            for (uri in uris) {
                val fileName = uri.getFileName() ?: "null"

                try {
                    val bitmapInfo = withContext(Dispatchers.IO) {
                        contentResolver.openFileDescriptor(uri, "r")!!.use {
                            loadBitmapWrapper(it.fd)
                        }
                    }

                    bitmapInfo.name = fileName
                    viewModel.stackingViewModel.bitmaps.add(bitmapInfo)
                    binding.loadedImagesCount.isVisible = true
                    binding.loadedImagesCount.text = "Images: ${viewModel.stackingViewModel.bitmaps.size}"
                    binding.loadedImagesList.addView(BitmapListItem(
                        this@StackingActivity,
                        bitmapInfo,
                        ::handleBitmapRemoved
                    ))

                    binding.logWindow.addMessage("Image: $fileName loaded")
                }
                catch (e: NullPointerException) {
                    binding.logWindow.addMessage("File $fileName does not exist or can't be accessed", true)
                }
                catch (e: Exception) {
                    binding.logWindow.addMessage("Failed reading file: $fileName - ${e.message}", true)
                }
            }
            endAction()
        }
    }
    private fun Uri.getFileName(): String? {
        contentResolver.query(this, null, null, null, null)?.use { cursor ->
            val nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            if (nameIndex != -1 && cursor.moveToFirst()) {
                return cursor.getString(nameIndex)
            }
        }
        return null
    }
    private fun loadBitmapWrapper(fd: Int): BitmapInfo {
        val packedData = loadBitmap(fd)
        val width = (packedData shr 32).toInt()
        val height = packedData.toInt()
        return BitmapInfo(width, height)
    }
    private fun handleBitmapRemoved(item: BitmapListItem) {
        val index = binding.loadedImagesList.indexOfChild(item)
        removeBitmap(index)
        viewModel.stackingViewModel.bitmaps.removeAt(index)
        binding.loadedImagesList.removeView(item)
        binding.loadedImagesCount.text = "Images: ${viewModel.stackingViewModel.bitmaps.size}"
        if (viewModel.stackingViewModel.bitmaps.isEmpty()) binding.loadedImagesCount.isVisible = false
        updateButtonsState()
    }

    private fun handleAnalyseButtonClicked() {
        startAction()
        viewModel.stackingViewModel.hasStackedResult = false

        lifecycleScope.launch {
            val scores = withContext(Dispatchers.Default) {
                analyse(binding.analyseCheckBox.isChecked)
            }
            binding.loadedImagesList.forEachIndexed { index, item ->
                val bitmapListItem = item as BitmapListItem
                bitmapListItem.bitmapInfo.score = scores[index]
                bitmapListItem.updateScore()
            }
            endAction()
        }
    }

    private fun handleStackButtonClicked() {
        startAction()

        lifecycleScope.launch {
            try {
                withContext(Dispatchers.Default) {
                    stack(binding.disableAlignmentCheckBox.isChecked)
                }
                viewModel.stackingViewModel.hasStackedResult = true
            }
            catch (e: RuntimeException) {
                binding.logWindow.addMessage("Error: ${e.message}", true)
            }
            endAction()
        }
    }

    private fun handleSaveButtonClicked() {
        startAction()

        lifecycleScope.launch {
            try {
                val ext = binding.outputFormatSelectorAutoCompleteTextView.text.toString()
                val timestamp = System.currentTimeMillis()
                val filename = "Stacked_Result_${timestamp}${ext}"
                val mimeType = when (ext) {
                    ".jpg" -> "image/jpeg"
                    ".png" -> "image/png"
                    else -> "image/*"
                }

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    // Android 10+ (API 29+)
                    val contentValues = ContentValues().apply {
                        put(MediaStore.MediaColumns.DISPLAY_NAME, filename)
                        put(MediaStore.MediaColumns.MIME_TYPE, mimeType)
                        put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_PICTURES + APP_DIRECTORY)
                    }

                    val imageUri = contentResolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues)
                    withContext(Dispatchers.IO) {
                        contentResolver.openFileDescriptor(imageUri!!, "w")!!.use {
                            save(it.fd, OUTPUT_FORMATS.indexOf(ext))
                        }
                    }
                } else {
                    // Android 9-
                    val picturesDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES)
                    val outputDir = File(picturesDir, APP_DIRECTORY)
                    if (!outputDir.exists()) outputDir.mkdirs()

                    val file = File(outputDir, filename)

                    withContext(Dispatchers.IO) {
                        val mode = ParcelFileDescriptor.MODE_WRITE_ONLY or ParcelFileDescriptor.MODE_CREATE or ParcelFileDescriptor.MODE_TRUNCATE
                        ParcelFileDescriptor.open(file, mode).use {
                            save(it.fd, OUTPUT_FORMATS.indexOf(ext))
                        }
                    }

                    MediaScannerConnection.scanFile(
                        this@StackingActivity,
                        arrayOf(file.absolutePath),
                        arrayOf(mimeType),
                        null
                    )
                }

                binding.logWindow.addMessage("Image was saved successfully")
            }
            catch (e: NullPointerException) {
                binding.logWindow.addMessage("Unexpected error. Image was not saved", true)
            }
            catch (e: RuntimeException) {
                binding.logWindow.addMessage("Error: ${e.message}", true)
            }
            endAction()
        }
    }

    private fun handleLogWindowsClosed() {
        binding.logWindow.isVisible = false
        binding.logWindowBackground.isVisible = false
    }

    // Functions which are called from native code
    fun createTempFile(): String {
        val tempFile = File.createTempFile("temp_", "", cacheDir)
        return tempFile.absolutePath
    }
    fun createImageFile(name: String): Int {
        val contentValues = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, name)
            put(MediaStore.MediaColumns.MIME_TYPE, "image/*")
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_PICTURES + APP_DIRECTORY)
            } else {
                val path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES).absolutePath + APP_DIRECTORY
                put(MediaStore.MediaColumns.DATA, path + name)
            }
        }

        val uri = contentResolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues)!!
        val pfd = contentResolver.openFileDescriptor(uri, "w")!! // pfd will be closed in native code

        return pfd.detachFd()
    }
    fun addLogMessage(message: String, isError: Boolean) {
        runOnUiThread {
            binding.logWindow.addMessage(message, isError)
        }
    }

    // Native functions
    external fun initStacking(am: AssetManager)
    external fun loadBitmap(fd: Int): Long
    external fun removeBitmap(index: Int)
    external fun analyse(reanalyse: Boolean): IntArray
    external fun stack(disableAlignment: Boolean)
    external fun save(fd: Int, format: Int)

    companion object {
        public const val APP_DIRECTORY = "/StackingCamera/"
        public const val PICK_IMAGE_REQUEST = 0
        val OUTPUT_FORMATS = listOf(".jpg", ".png", ".tiff")
        init {
            System.loadLibrary("stackingcamera")
        }
    }
}