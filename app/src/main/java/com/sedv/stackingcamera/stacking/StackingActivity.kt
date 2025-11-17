package com.sedv.stackingcamera.stacking

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.sedv.stackingcamera.databinding.ActivityStackingBinding

class StackingActivity : AppCompatActivity() {
    private lateinit var binding: ActivityStackingBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityStackingBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.navToCameraButton.setOnClickListener {
            finish()
        }
    }
}