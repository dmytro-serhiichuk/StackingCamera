package com.sedv.stackingcamera.main

import android.annotation.SuppressLint
import android.content.Context
import android.os.CountDownTimer
import android.view.MotionEvent
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.widget.AppCompatButton
import androidx.core.view.isVisible
import com.sedv.stackingcamera.main.camera.CameraError
import com.sedv.stackingcamera.main.camera.CameraState
import com.sedv.stackingcamera.main.generalsettings.property.BaseProperty
import com.sedv.stackingcamera.main.generalsettings.GeneralPropertyType
import com.sedv.stackingcamera.main.generalsettings.GeneralSettings
import kotlin.math.pow
import kotlin.math.round
import kotlin.math.sqrt

@SuppressLint("ClickableViewAccessibility")
class ShutterController(
    private val context: Context,
    private val viewModel: CameraViewModel,
    private val shutter: AppCompatButton,
    private val additionalShutter: AppCompatButton,
    private val screenIndicator: TextView
) {
    private var timer: Int = 0
    private var countdown: CountDownTimer? = null

    private var remainedBurstNumber = 0

    private var isAdditionalShutterDragging = false
    private var startPosition = Pair(0f, 0f)
    private var dragOffset = Pair(0f, 0f)

    init {
        viewModel.onCameraSwitched += ::handleCameraSwitched
        viewModel.onProgramReady += ::handleProgramReady

        GeneralSettings.onChanged += ::handleGeneralPropertyChanged

        shutter.setOnClickListener {
            handleShutterPress()
        }

        shutter.setOnLongClickListener {
            if (viewModel.activeCamera.currentState == CameraState.OPENED) {
                additionalShutter.isVisible = true
                additionalShutter.x = shutter.x
                additionalShutter.y = shutter.y
            }
            true
        }
        additionalShutter.setOnTouchListener { view, event ->
            when (event.action) {
                MotionEvent.ACTION_DOWN -> {
                    isAdditionalShutterDragging = false

                    startPosition = Pair(view.x, view.y)
                    dragOffset = Pair(event.rawX - view.x, event.rawY - view.y)

                    true
                }
                MotionEvent.ACTION_MOVE -> {
                    val deltaX = event.rawX - (startPosition.first + dragOffset.first)
                    val deltaY = event.rawY - (startPosition.second + dragOffset.second)
                    val distance = sqrt(deltaX * deltaX + deltaY * deltaY)

                    if (distance >= MIN_DRAG_DISTANCE) {
                        isAdditionalShutterDragging = true
                        additionalShutter.x = event.rawX - dragOffset.first
                        additionalShutter.y = event.rawY - dragOffset.second
                    }

                    true
                }
                MotionEvent.ACTION_UP -> {
                    val distance = getDistanceBetweenShutterButtons()
                    if (distance <= shutter.width * 1.2) {
                        additionalShutter.isVisible = false
                        if (distance == 0f) handleShutterPress()
                    } else {
                        if (!isAdditionalShutterDragging) {
                            handleShutterPress()
                        }

                        isAdditionalShutterDragging = false
                    }

                    true
                }
                else -> false
            }
        }
    }

    private fun getDistanceBetweenShutterButtons(): Float {
        val x1 = shutter.x + shutter.width / 2
        val y1 = shutter.y + shutter.height / 2

        val x2 = additionalShutter.x + additionalShutter.width / 2
        val y2 = additionalShutter.y + additionalShutter.height / 2

        return sqrt((x2 - x1).pow(2) + (y2 - y1).pow(2))
    }

    private fun handleCameraSwitched() {
        viewModel.activeCamera.onPhotoReceived += {
            if (remainedBurstNumber > 0) {
                remainedBurstNumber--
                startShutterCountDown()
            }
        }
    }
    private fun handleProgramReady() {
        timer = GeneralSettings.timer.value
        handleCameraSwitched()
    }

    private fun handleShutterPress() {
        if (viewModel.activeCamera.currentState != CameraState.OPENED) return

        if (countdown != null) {
            countdown?.cancel()
            countdown = null
            screenIndicator.text = ""
            takePhoto()
        } else {
            if (timer > 0) startCountdown()
            else takePhoto()
        }
    }

    private fun startCountdown() {
        screenIndicator.text = timer.toString()

        countdown = object : CountDownTimer(timer * 1000L, 1000L) {
            override fun onTick(millisUntilFinished: Long) {
                val secondsLeft = (millisUntilFinished / 1000).toInt()
                screenIndicator.text = (secondsLeft + 1).toString()
            }

            override fun onFinish() {
                screenIndicator.text = ""
                countdown?.cancel()
                countdown = null
                takePhoto()
            }
        }.start()
    }

    private fun takePhoto() {
        if (viewModel.permissionHelper.hasAllPermissions()) {
            try {
                val burstProperty = viewModel.activeCamera.cameraSettings.burstProperty
                if (burstProperty != null && burstProperty.value > 1) {
                    remainedBurstNumber = burstProperty.value - 1
                    viewModel.activeCamera.takeBurst()
                } else {
                    viewModel.activeCamera.takePicture()
                }

                startShutterCountDown()

            } catch (e: CameraError) {
                Toast.makeText(context, e.message, Toast.LENGTH_SHORT).show()
            }
        } else {
            viewModel.permissionHelper.requestAllPermissions()
        }
    }

    private fun handleGeneralPropertyChanged(prop: BaseProperty<*>) {
        if (prop.type == GeneralPropertyType.TIMER) {
            timer = prop.value as Int
        }
    }

    private fun startShutterCountDown() {
        viewModel.activeCamera.cameraSettings.exposureTimeNS?.let { speed ->
            if (speed.value >= LONG_EXPOSURE) {
                shutter.text = (speed.value / LONG_EXPOSURE).toString()

                countdown = object : CountDownTimer(speed.value / 1000000, 100L) {
                    override fun onTick(millisUntilFinished: Long) {
                        val secondsLeft = round(millisUntilFinished / 100.0) / 10
                        shutter.text = secondsLeft.toString()
                    }

                    override fun onFinish() {
                        shutter.text = ""
                        countdown?.cancel()
                        countdown = null
                    }
                }.start()
            }
        }
    }

    companion object {
        private const val MIN_DRAG_DISTANCE = 20f
        private const val LONG_EXPOSURE = 1_000_000_000L
    }
}