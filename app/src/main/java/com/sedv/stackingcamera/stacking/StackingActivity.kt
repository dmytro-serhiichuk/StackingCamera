package com.sedv.stackingcamera.stacking

import android.annotation.SuppressLint
import android.content.ContentValues
import android.content.Intent
import android.content.res.AssetManager
import android.os.Build
import android.os.Bundle
import android.os.Environment
import android.provider.MediaStore
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import com.sedv.stackingcamera.databinding.ActivityStackingBinding
import com.sedv.stackingcamera.stacking.settings.StackingSettingsActivity
import java.io.File

class StackingActivity : AppCompatActivity() {
    private lateinit var binding: ActivityStackingBinding

    private val viewModel: StackingViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityStackingBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.navToCameraButton.setOnClickListener {
            finish()
        }

        binding.navToStackingSettingsButton.setOnClickListener {
            val intent = Intent(this, StackingSettingsActivity::class.java)
            startActivity(intent)
        }
    }


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

    external fun initStacking(am: AssetManager)

    companion object {
        public const val APP_FOLDER = "/StackingCamera/"
        init {
            System.loadLibrary("stackingcamera")
        }
    }
}