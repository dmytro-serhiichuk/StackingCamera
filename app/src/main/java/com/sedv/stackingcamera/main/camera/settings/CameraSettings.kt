package com.sedv.stackingcamera.main.camera.settings

import com.sedv.stackingcamera.main.camera.CameraInfo

data class CameraSettings(
    val cameraInfo: CameraInfo,
    val onSettingsManuallyChanged: () -> Unit
) {
    val properties: Set<BaseSettingsProperty<*>>

    val format: FormatProperty?
    val iso: ISOProperty?
    val exposureTimeNS: ShutterProperty?
    val ev: EVProperty?
    val whiteBalance: WhiteBalanceProperty?
    val focusModes: FocusModesProperty?
    val manualFocus: ManualFocusProperty?
    val burstProperty: BurstProperty?

    val meteringArea: MeteringArea?
    val zoomProperty: ZoomProperty?

    val aeState: AEState
        get() {
            return when {
                iso == null && exposureTimeNS == null -> {
                    AEState.UNSUPPORTED
                }
                iso != null && exposureTimeNS != null -> {
                    if (iso.isInAutoMode && exposureTimeNS.isInAutoMode) AEState.AUTO
                    else if (iso.isInAutoMode || exposureTimeNS.isInAutoMode) AEState.SEMI_AUTO
                    else AEState.MANUAL
                }
                iso != null -> {
                    if (iso.isInAutoMode) AEState.AUTO
                    else AEState.MANUAL
                }
                else -> {
                    if (exposureTimeNS!!.isInAutoMode) AEState.AUTO
                    else AEState.MANUAL
                }
            }
        }

    init {
        properties = mutableSetOf<BaseSettingsProperty<*>>()

        format = if (cameraInfo.hasRawCapture) {
            val prop = FormatProperty(cameraInfo, onSettingsManuallyChanged)
            properties.add(prop)
            prop
        } else null
        iso = if (cameraInfo.hasManualSensor && cameraInfo.isoRange.lower != cameraInfo.isoRange.upper) {
            val prop = ISOProperty(cameraInfo, onSettingsManuallyChanged)
            properties.add(prop)
            prop
        } else null
        exposureTimeNS = if (cameraInfo.hasManualSensor && cameraInfo.exposureRange.lower != cameraInfo.exposureRange.upper) {
            val prop = ShutterProperty(cameraInfo, onSettingsManuallyChanged)
            properties.add(prop)
            prop
        } else null
        ev = if (cameraInfo.evRange.lower != cameraInfo.evRange.upper) {
            val prop = EVProperty(cameraInfo, onSettingsManuallyChanged)
            properties.add(prop)
            prop
        } else null
        whiteBalance = if (cameraInfo.awbModes.size > 1) {
            val prop = WhiteBalanceProperty(cameraInfo, onSettingsManuallyChanged)
            properties.add(prop)
            prop
        } else null
        focusModes = if (cameraInfo.afModes.size > 1) {
            val prop = FocusModesProperty(cameraInfo, onSettingsManuallyChanged)
            properties.add(prop)
            prop
        } else null
        manualFocus = if (focusModes != null && cameraInfo.hasManualFocus && cameraInfo.hasManualSensor) {
            val prop = ManualFocusProperty(cameraInfo, onSettingsManuallyChanged)
            properties.add(prop)
            prop
        } else null
        burstProperty = if (cameraInfo.hasBurst) {
            val prop = BurstProperty(cameraInfo, onSettingsManuallyChanged)
            properties.add(prop)
            prop
        } else null

        meteringArea = if (focusModes != null && cameraInfo.afRegions > 0) {
            val prop = MeteringArea(
                cameraInfo,
                onSettingsManuallyChanged,
                cameraInfo.aeRegions > 0,
                cameraInfo.awbRegions > 0
            )
            prop
        } else null
        zoomProperty = if (cameraInfo.maxZoom > 0f) {
            val prop = ZoomProperty(cameraInfo, onSettingsManuallyChanged)
            prop
        } else null
    }

    companion object {
        const val MAX_BURST_IMAGES = 20
    }
}

enum class AEState {
    AUTO,
    SEMI_AUTO,
    MANUAL,
    UNSUPPORTED
}