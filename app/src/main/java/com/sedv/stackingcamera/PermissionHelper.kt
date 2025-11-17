package com.sedv.stackingcamera

import android.Manifest
import android.app.Activity
import android.app.AlertDialog
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class PermissionHelper(private val activity: Activity) {
    companion object {
        const val STORAGE_PERMISSION_REQUEST_CODE = 100
        const val CAMERA_PERMISSION_REQUEST_CODE = 101
        const val ALL_PERMISSIONS_REQUEST_CODE = 102
    }

    fun hasCameraPermission(): Boolean {
        return ContextCompat.checkSelfPermission(
            activity,
            Manifest.permission.CAMERA
        ) == PackageManager.PERMISSION_GRANTED
    }

    fun hasStoragePermission(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13+ (API 33+)
            ContextCompat.checkSelfPermission(
                activity,
                Manifest.permission.READ_MEDIA_IMAGES
            ) == PackageManager.PERMISSION_GRANTED
        } else if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            // Android 10-12 (API 29-32)
            true
        } else {
            // Android 9- (API 28-)
            ContextCompat.checkSelfPermission(
                activity,
                Manifest.permission.WRITE_EXTERNAL_STORAGE
            ) == PackageManager.PERMISSION_GRANTED &&
            ContextCompat.checkSelfPermission(
                activity,
                Manifest.permission.READ_EXTERNAL_STORAGE
            ) == PackageManager.PERMISSION_GRANTED
        }
    }

    fun hasAllPermissions(): Boolean {
        return hasCameraPermission() && hasStoragePermission()
    }

    fun requestCameraPermission() {
        ActivityCompat.requestPermissions(
            activity,
            arrayOf(Manifest.permission.CAMERA),
            CAMERA_PERMISSION_REQUEST_CODE
        )
    }

    fun requestStoragePermission() {
        val permissions = getRequiredStoragePermissions()
        if (permissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(
                activity,
                permissions,
                STORAGE_PERMISSION_REQUEST_CODE
            )
        }
    }

    fun requestAllPermissions() {
        val permissions = mutableListOf<String>()

        if (!hasCameraPermission()) {
            permissions.add(Manifest.permission.CAMERA)
        }

        if (!hasStoragePermission()) {
            permissions.addAll(getRequiredStoragePermissions())
        }

        if (permissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(
                activity,
                permissions.toTypedArray(),
                ALL_PERMISSIONS_REQUEST_CODE
            )
        }
    }

    private fun getRequiredStoragePermissions(): Array<String> {
        return when {
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU -> {
                // Android 13+ (API 33+)
                arrayOf(Manifest.permission.READ_MEDIA_IMAGES)
            }
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q -> {
                // Android 10-12 (API 29-32)
                emptyArray()
            }
            else -> {
                // Android 9- (API 28-)
                arrayOf(
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE
                )
            }
        }
    }

    fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray,
        onResult: (Boolean) -> Unit
    ) {
        when (requestCode) {
            CAMERA_PERMISSION_REQUEST_CODE -> {
                val granted = grantResults.isNotEmpty() &&
                        grantResults[0] == PackageManager.PERMISSION_GRANTED

                if (granted) {
                    onResult(true)
                } else {
                    showPermissionDeniedDialog { onResult(false) }
                }
            }

            STORAGE_PERMISSION_REQUEST_CODE -> {
                val allGranted = grantResults.isNotEmpty() &&
                        grantResults.all { it == PackageManager.PERMISSION_GRANTED }

                if (allGranted) {
                    onResult(true)
                } else {
                    showPermissionDeniedDialog { onResult(false) }
                }
            }

            ALL_PERMISSIONS_REQUEST_CODE -> {
                val allGranted = grantResults.isNotEmpty() &&
                        grantResults.all { it == PackageManager.PERMISSION_GRANTED }

                if (allGranted) {
                    onResult(true)
                } else {
                    showPermissionDeniedDialog {
                        onResult(false) }
                }
            }
        }
    }

    private fun showPermissionDeniedDialog(
        onDismiss: (() -> Unit)? = null
    ) {
        AlertDialog.Builder(activity)
            .setTitle("Required permissions")
            .setMessage("The app requires Camera and Storage permissions to work properly. You can grant them in the settings")
            .setPositiveButton("Open Settings") { dialog, _ ->
                dialog.dismiss()
                openAppSettings()
                if (activity is MainActivity) {
                    activity.onReturnFromSettings()
                }
                onDismiss?.invoke()
            }
            .setNegativeButton("Exit") { dialog, _ ->
                dialog.dismiss()
                activity.finish()
            }
            .setCancelable(false)
            .show()
    }

    private fun openAppSettings() {
        val intent = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS).apply {
            data = Uri.fromParts("package", activity.packageName, null)
        }
        activity.startActivity(intent)
    }
}