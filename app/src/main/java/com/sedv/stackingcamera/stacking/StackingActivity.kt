package com.sedv.stackingcamera.stacking

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
import android.util.Log
import android.view.WindowManager
import android.widget.ArrayAdapter
import android.widget.AutoCompleteTextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.forEach
import androidx.core.view.forEachIndexed
import androidx.core.view.get
import androidx.core.view.isVisible
import androidx.lifecycle.lifecycleScope
import com.sedv.stackingcamera.databinding.ActivityStackingBinding
import com.sedv.stackingcamera.stacking.settings.Settings
import com.sedv.stackingcamera.stacking.settings.StackingSettingsActivity
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import kotlin.system.exitProcess

class StackingActivity : AppCompatActivity() {
    private lateinit var binding: ActivityStackingBinding

    private val viewModel: StackingViewModel by viewModels()

    private val pickImagesLauncher = registerForActivityResult(ActivityResultContracts.GetMultipleContents())
    { uris: List<Uri> ->
        if (uris.isNotEmpty()) {
            loadImages(uris)
        } else {
            endAction()
            handleLogWindowsClosed()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityStackingBinding.inflate(layoutInflater)
        setContentView(binding.root)

        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)

        updateButtonsState()

        if (StackingHandler.state == StackingState.NOT_READY) {
            cacheDir.listFiles()?.forEach { f ->
                f.delete()
            }
            lifecycleScope.launch {
                try {
                    withContext(Dispatchers.IO) {
                        initStacking(applicationContext.assets)
                        Settings.initialize(getSharedPreferences("SETTINGS", MODE_PRIVATE))
                    }
                    StackingHandler.state = StackingState.IDLE
                    updateButtonsState()
                    val uris = intent.getParcelableArrayListExtra<Uri>(Intent.EXTRA_STREAM)
                    if (!uris.isNullOrEmpty()) {
                        startAction()
                        loadImages(uris)
                    }
                } catch (e: Exception) {
                    showFatalError(e)
                }
            }
        } else {
            updateButtonsState()
            val uris = intent.getParcelableArrayListExtra<Uri>(Intent.EXTRA_STREAM)
            if (!uris.isNullOrEmpty()) {
                startAction()
                loadImages(uris)
            }
        }

        viewModel.initPermissionHelper(this)

        binding.navToCameraButton.setOnClickListener {
            finish()
        }

        binding.navToStackingSettingsButton.setOnClickListener {
            val intent = Intent(this, StackingSettingsActivity::class.java)
            startActivity(intent)
        }

        binding.loadImagesBtn.setOnClickListener { handleLoadImagesButtonClicked() }
        binding.analyseBtn.setOnClickListener { handleAnalyseButtonClicked() }
        binding.matchBtn.setOnClickListener { handleMatchButtonClicked() }
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

        for (bitmap in StackingHandler.bitmaps) {
            binding.loadedImagesList.addView(BitmapListItem(
                this, bitmap, ::handleBitmapRemoved
            ))
        }
        if (StackingHandler.bitmaps.isNotEmpty()) {
            binding.loadedImagesCount.isVisible = true
            binding.loadedImagesCount.text = "Images: ${StackingHandler.bitmaps.size}"
        }
    }

    override fun onResume() {
        super.onResume()

        updateButtonsState()
        // TODO: handle image reloading after changing bit depth or color space
        if (StackingHandler.isAnalyseResultsRemoved) {
            StackingHandler.bitmaps.forEach { it.bitmapValidationInfo = null }
            binding.loadedImagesList.forEach {
                val bli = it as BitmapListItem
                bli.updateAdditionalInfoText()
                bli.updateReferenceFrameLabel()
                bli.updateWarningMessage()
            }
            StackingHandler.isAnalyseResultsRemoved = false
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
        val isIdle = StackingHandler.state == StackingState.IDLE
        binding.loadImagesBtn.isEnabled = isIdle
        binding.analyseBtn.isEnabled = isIdle && StackingHandler.bitmaps.isNotEmpty()
        binding.matchBtn.isEnabled = isIdle && StackingHandler.hasEnoughBitmaps() && StackingHandler.isAllBitmapsInitialized() && !binding.disableAlignmentCheckBox.isChecked
        binding.stackBtn.isEnabled = isIdle && StackingHandler.hasEnoughBitmaps() && ((StackingHandler.isAllBitmapsInitialized() && StackingHandler.hasMatches) || binding.disableAlignmentCheckBox.isChecked)
        binding.saveBtn.isEnabled = isIdle && StackingHandler.hasStackedResult
        binding.loadedImagesList.forEach {
            val bitmapListItem = it as BitmapListItem
            bitmapListItem.setEnableMode(isIdle)
        }
        if (StackingHandler.state == StackingState.BUSY) {
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
        StackingHandler.state = StackingState.BUSY
        updateButtonsState()
    }
    private fun endAction() {
        StackingHandler.state = StackingState.IDLE
        updateButtonsState()
    }

    private fun handleLoadImagesButtonClicked() {
        startAction()
        pickImagesLauncher.launch("*/*")
    }

    private fun loadImages(uris: List<Uri>) {
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
                    StackingHandler.bitmaps.add(bitmapInfo)
                    binding.loadedImagesCount.isVisible = true
                    binding.loadedImagesCount.text = "Images: ${StackingHandler.bitmaps.size}"
                    binding.loadedImagesList.addView(BitmapListItem(
                        this@StackingActivity,
                        bitmapInfo,
                        ::handleBitmapRemoved
                    ))

                    removeReferenceFrame()
                    removeMatchesInfo()

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
    private fun removeMatchesInfo() {
        StackingHandler.hasMatches = false
        StackingHandler.bitmaps.forEach { it.bitmapValidationInfo = null }
        binding.loadedImagesList.forEach { (it as BitmapListItem).updateWarningMessage() }
    }
    private fun handleBitmapRemoved(item: BitmapListItem) {
        val allWereInitialized = StackingHandler.isAllBitmapsInitialized()

        val index = binding.loadedImagesList.indexOfChild(item)
        removeBitmap(index)
        val bitmapInfo = StackingHandler.bitmaps[index]
        val isReferenceFrame = bitmapInfo == StackingHandler.referenceBitmap
        StackingHandler.bitmaps.remove(bitmapInfo)
        binding.loadedImagesList.removeView(item)
        binding.loadedImagesCount.text = "Images: ${StackingHandler.bitmaps.size}"
        if (StackingHandler.bitmaps.isEmpty()) binding.loadedImagesCount.isVisible = false

        updateButtonsState()

        if (isReferenceFrame) {
            StackingHandler.referenceBitmap = null
            removeMatchesInfo()
            updateButtonsState()
        }
        if ((!allWereInitialized && StackingHandler.isAllBitmapsInitialized()) || (allWereInitialized && isReferenceFrame)) {
            lifecycleScope.launch {
                updateReferenceFrame()
            }
        }
    }

    private fun handleAnalyseButtonClicked() {
        if (!binding.analyseCheckBox.isChecked && StackingHandler.isAllBitmapsInitialized()) return

        startAction()
        removeMatchesInfo()
        StackingHandler.hasStackedResult = false

        lifecycleScope.launch {
            val scores = withContext(Dispatchers.Default) {
                analyse(binding.analyseCheckBox.isChecked)
            }
            binding.loadedImagesList.forEachIndexed { index, item ->
                val bitmapListItem = item as BitmapListItem
                bitmapListItem.bitmapInfo.score = scores[index]
                bitmapListItem.updateAdditionalInfoText()
            }
            updateReferenceFrame()
            endAction()
        }
    }
    private suspend fun updateReferenceFrame() {
        val index = withContext(Dispatchers.Default) {
            getIndexOfReferenceFrame()
        }
        if (StackingHandler.bitmaps.isEmpty()) return

        removeReferenceFrame()

        val bitmapInfo = StackingHandler.bitmaps[index]
        bitmapInfo.isReferenceFrame = true

        StackingHandler.referenceBitmap = bitmapInfo
        (binding.loadedImagesList[index] as BitmapListItem).updateReferenceFrameLabel()
    }
    private fun removeReferenceFrame() {
        StackingHandler.referenceBitmap?.let {
            it.isReferenceFrame = false
            val oldIndex = StackingHandler.bitmaps.indexOf(it)
            (binding.loadedImagesList[oldIndex] as BitmapListItem).updateReferenceFrameLabel()
        }
    }

    private fun handleMatchButtonClicked() {
        startAction()
        StackingHandler.hasStackedResult = false

        lifecycleScope.launch {
            try {
                val flat = withContext(Dispatchers.Default) {
                    match()
                }
                StackingHandler.hasMatches = true

                flat.toList().chunked(BitmapValidationInfo.FIELDS_PER_MATCH_RESULT) { chunk ->
                    StackingHandler.bitmaps[chunk[0]].apply {
                        bitmapValidationInfo = BitmapValidationInfo(
                            BitmapValidationStatus.fromInt(chunk[1]),
                            BitmapValidationStatus.fromInt(chunk[2]),
                            BitmapValidationStatus.fromInt(chunk[3]),
                            BitmapValidationStatus.fromInt(chunk[4]),
                            BitmapValidationStatus.fromInt(chunk[5]),
                            BitmapValidationStatus.fromInt(chunk[6]),
                            chunk[7] != 0,
                            chunk[8] != 0,
                            BitmapValidationStatus.fromInt(chunk[9]),
                            BitmapValidationStatus.fromInt(chunk[10]),
                            BitmapValidationStatus.fromInt(chunk[11]),
                        )
                        (binding.loadedImagesList[chunk[0]] as BitmapListItem).updateWarningMessage()
                    }
                }
            }
            catch (e: RuntimeException) {
                binding.logWindow.addMessage("Error: ${e.message}", true)
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
                StackingHandler.hasStackedResult = true
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
                        val fd = contentResolver.openFileDescriptor(imageUri!!, "w")!!.detachFd()
                        save(fd, OUTPUT_FORMATS.indexOf(ext))
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
    external fun getIndexOfReferenceFrame(): Int
    external fun match(): IntArray
    external fun stack(disableAlignment: Boolean)
    external fun save(fd: Int, format: Int)

    companion object {
        const val APP_DIRECTORY = "/StackingCamera/"
        val OUTPUT_FORMATS = listOf(".jpg", ".png", ".tiff")
        init {
            System.loadLibrary("stackingcamera")
        }
    }
}