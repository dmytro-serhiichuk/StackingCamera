package com.sedv.stackingcamera.stacking

import android.content.Intent
import android.content.res.AssetManager
import android.os.Bundle
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import com.sedv.stackingcamera.databinding.ActivityStackingBinding
import com.sedv.stackingcamera.stacking.settings.StackingSettingsActivity

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

    external fun initStacking(am: AssetManager)

    companion object {
        init {
            System.loadLibrary("stackingcamera")
        }
    }
}