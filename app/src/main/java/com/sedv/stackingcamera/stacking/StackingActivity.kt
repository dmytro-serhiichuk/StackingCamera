package com.sedv.stackingcamera.stacking

import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.content.ContentValues
import android.content.Intent
import android.content.res.AssetManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.MediaStore
import android.provider.OpenableColumns
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.forEachIndexed
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (viewModel.state == StackingState.NOT_READY) {
            cacheDir.listFiles()?.forEach { f ->
                f.delete()
            }
            lifecycleScope.launch {
                try {
                    withContext(Dispatchers.IO) {
                        initStacking(applicationContext.assets)
                        Settings.initialize(getSharedPreferences("SETTINGS", MODE_PRIVATE))
                    }
                    viewModel.state = StackingState.IDLE
                    updateButtonsState()
                } catch (e: Exception) {
                    showFatalError(e)
                }
            }
        }

        binding = ActivityStackingBinding.inflate(layoutInflater)
        setContentView(binding.root)

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

        val outputFormatAdapter = ArrayAdapter(
            this,
            android.R.layout.simple_spinner_item,
            listOf(".jpg", ".png", ".tiff")
        )
        outputFormatAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.outputFormatSelector.adapter = outputFormatAdapter
        binding.outputFormatSelector.setSelection(0)
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
        val isIdle = viewModel.state == StackingState.IDLE
        binding.loadImagesBtn.isEnabled = isIdle
        binding.analyseBtn.isEnabled = isIdle && viewModel.bitmaps.isNotEmpty()
        binding.stackBtn.isEnabled = isIdle && viewModel.canStack()
        binding.saveBtn.isEnabled = isIdle && viewModel.hasStackedResult
    }
    private fun startAction() {
        viewModel.state = StackingState.BUSY
        updateButtonsState()
    }
    private fun endAction() {
        viewModel.state = StackingState.IDLE
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
        if (requestCode == PICK_IMAGE_REQUEST && resultCode == Activity.RESULT_OK) {
            loadImages(data)
        }
    }

    private fun loadImages(data: Intent?) {
        if (data == null || (data.data == null && data.clipData == null)) return

        val selectedFiles = arrayListOf<Uri>()
        data.clipData?.let { clip ->
            repeat(clip.itemCount) {
                selectedFiles.add(clip.getItemAt(it).uri)
            }
        } ?: data.data?.let {
            selectedFiles.add(it)
        }

        lifecycleScope.launch {
            for (file in selectedFiles) {
                val fileName = file.getFileName() ?: "null"

                try {
                    val bitmapInfo = withContext(Dispatchers.IO) {
                        contentResolver.openFileDescriptor(file, "r")!!.use {
                            loadBitmapWrapper(it.fd)
                        }
                    }

                    bitmapInfo.name = fileName
                    viewModel.bitmaps.add(bitmapInfo)
                    binding.loadedImagesCount.isVisible = true
                    binding.loadedImagesCount.text = "Images: ${viewModel.bitmaps.size}"
                    binding.loadedImagesList.addView(BitmapListItem(
                        this@StackingActivity,
                        bitmapInfo,
                        ::handleBitmapRemoved
                    ))
                }
                catch (e: NullPointerException) {
                    Toast.makeText(
                        this@StackingActivity,
                        "File: $fileName does not exist or can't be accessed",
                        Toast.LENGTH_LONG
                    ).show()
                }
                catch (e: Exception) {
                    Toast.makeText(
                        this@StackingActivity,
                        "Failed reading file: $fileName - ${e.message}",
                        Toast.LENGTH_LONG
                    ).show()
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
        viewModel.bitmaps.removeAt(index)
        binding.loadedImagesList.removeView(item)
        binding.loadedImagesCount.text = "Images: ${viewModel.bitmaps.size}"
        if (viewModel.bitmaps.isEmpty()) binding.loadedImagesCount.isVisible = false
        updateButtonsState()
    }

    private fun handleAnalyseButtonClicked() {
        startAction()

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

    // Functions which are called from native code
    fun createTempFile(): String {
        val tempFile = File.createTempFile("temp_", "", cacheDir)
        return tempFile.absolutePath
    }
    @SuppressLint("Recycle")
    fun createImageFile(name: String): Int {
        val contentValues = ContentValues().apply {
            put(MediaStore.MediaColumns.DISPLAY_NAME, name)
            put(MediaStore.MediaColumns.MIME_TYPE, "image")
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                put(MediaStore.MediaColumns.RELATIVE_PATH, Environment.DIRECTORY_PICTURES + APP_FOLDER)
            } else {
                val path = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES).absolutePath + APP_FOLDER
                put(MediaStore.MediaColumns.DATA, path + name)
            }
        }

        val uri = contentResolver.insert(MediaStore.Images.Media.EXTERNAL_CONTENT_URI, contentValues)!!
        val pfd = contentResolver.openFileDescriptor(uri, "w")!! // pfd will be closed in native code

        return pfd.fd
    }

    // Native functions
    external fun initStacking(am: AssetManager)
    external fun loadBitmap(fd: Int): Long
    external fun removeBitmap(index: Int)
    external fun analyse(reanalyse: Boolean): IntArray
    external fun stack(disableAlignment: Boolean)
    external fun save(fd: Int, format: Int)

    companion object {
        public const val APP_FOLDER = "/StackingCamera/"
        public const val PICK_IMAGE_REQUEST = 0
        init {
            System.loadLibrary("stackingcamera")
        }
    }
}