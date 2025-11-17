package com.sedv.stackingcamera.stacking.settings

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.sedv.stackingcamera.databinding.ActivityStackingSettingsBinding

class StackingSettingsActivity : AppCompatActivity() {
    private lateinit var binding: ActivityStackingSettingsBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityStackingSettingsBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.navBackButton.setOnClickListener {
            finish()
        }
    }
}