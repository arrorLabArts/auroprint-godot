package com.madeofcode.auroprint

import android.app.Activity
import android.util.Log
import org.godotengine.godot.Godot
import org.godotengine.godot.plugin.GodotPlugin
import org.godotengine.godot.plugin.UsedByGodot

/**
 * Godot Android Plugin for Auroprint GDExtension
 * This plugin provides JNI environment access to the GDExtension layer
 * and loads the native library containing the auroprint implementation.
 */
class AuroprintGodotPlugin(godot: Godot): GodotPlugin(godot) {

    companion object {
        private val TAG = AuroprintGodotPlugin::class.java.simpleName
        private var instance: AuroprintGodotPlugin? = null

        init {
            try {
                Log.v(TAG, "Loading auroprint library")
                System.loadLibrary("auroprint")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Unable to load auroprint shared library", e)
            }
        }

        /**
         * Get the singleton instance of this plugin
         * Called from native code to access the Android context and JNI environment
         */
        @JvmStatic
        fun getInstance(): AuroprintGodotPlugin? = instance
    }

    init {
        instance = this
        // Initialize native JNI environment
        nativeInitialize()
    }

    override fun getPluginName() = "auroprint"

    override fun getPluginGDExtensionLibrariesPaths() = setOf("res://addons/auroprint/auroprint.gdextension")

    /**
     * Get the Android Activity
     * Used by the JNI bridge to access Android context
     * Note: This shadows the protected getActivity() from GodotPlugin, but provides public access for JNI
     */
    fun getAndroidActivity(): Activity? = godot.getActivity()

    /**
     * Native method to initialize JNI environment
     * Called from constructor to pass JNI context to native code
     */
    private external fun nativeInitialize()
}
