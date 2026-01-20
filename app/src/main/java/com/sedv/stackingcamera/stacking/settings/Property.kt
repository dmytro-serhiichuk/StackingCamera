package com.sedv.stackingcamera.stacking.settings

import android.content.SharedPreferences
import android.util.Range

abstract class Property<T>(
    val name: String,
    val defaultValue: T,
    open var value: T = defaultValue
) {
    abstract fun loadFrom(sharedPreferences: SharedPreferences)
    abstract fun saveTo(editor: SharedPreferences.Editor)
    open fun reset(editor: SharedPreferences.Editor) {
        value = defaultValue
        saveTo(editor)
    }
}

class RangedProperty<T : Comparable<T>>(
    name: String,
    defaultValue: T,
    val range: Range<T>,
    val step: T
) : Property<T>(name, defaultValue) {
    override fun loadFrom(sharedPreferences: SharedPreferences) {
        value = when (defaultValue) {
            is Int -> sharedPreferences.getInt(name, defaultValue) as T
            is Float -> sharedPreferences.getFloat(name, defaultValue) as T
            else -> defaultValue
        }
    }

    override fun saveTo(editor: SharedPreferences.Editor) {
        when (value) {
            is Int -> editor.putInt(name, value as Int)
            is Float -> editor.putFloat(name, value as Float)
        }
    }
}

class BoolProperty(
    name: String,
    defaultValue: Boolean,
) : Property<Boolean>(name, defaultValue) {
    override fun loadFrom(sharedPreferences: SharedPreferences) {
        value = sharedPreferences.getBoolean(name, defaultValue)
    }

    override fun saveTo(editor: SharedPreferences.Editor) {
        editor.putBoolean(name, value)
    }
}

class OptionsProperty(
    name: String,
    defaultValue: Int,
    val options: LinkedHashSet<Int>
) : Property<Int>(name, defaultValue) {

    override fun loadFrom(sharedPreferences: SharedPreferences) {
        value = sharedPreferences.getInt(name, defaultValue)
    }

    override fun saveTo(editor: SharedPreferences.Editor) {
        editor.putInt(name, value)
    }
}