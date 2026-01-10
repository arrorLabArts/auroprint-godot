package com.madeofcode.auroprint

import android.content.Context
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit

/**
 * Synchronous wrapper for Auroprint functionality.
 * Used by JNI from C++ GDExtension.
 * All methods are blocking and return results directly.
 */
class AuroprintPluginSync(context: Context) {

    private val core = AuroprintCore(context)

    companion object {
        private const val TIMEOUT_SECONDS = 30L
    }

    /**
     * Generate auroprint synchronously.
     * This blocks until generation is complete.
     * Returns a HashMap with auroprint data.
     * Throws exception on error.
     */
    fun generateAuroprintSync(): HashMap<String, Any> {
        return try {
            core.generateAuroprint() as HashMap<String, Any>
        } catch (e: Exception) {
            throw Exception("Failed to generate auroprint: ${e.message}", e)
        }
    }

    /**
     * Check if hardware-backed keystore is available.
     * Returns true if available, false otherwise.
     */
    fun isHardwareBackedAvailableSync(): Boolean {
        return try {
            core.isHardwareBackedAvailable()
        } catch (e: Exception) {
            false
        }
    }

    /**
     * Reset the signing key synchronously.
     * Throws exception on error.
     */
    fun resetKeySync() {
        try {
            core.resetKey()
        } catch (e: Exception) {
            throw Exception("Failed to reset key: ${e.message}", e)
        }
    }

    /**
     * Request Play Integrity token synchronously.
     * This blocks until the token is received.
     * Returns the token string.
     * Throws exception on error.
     */
    fun requestIntegrityTokenSync(nonce: String, cloudProjectNumber: Long): String {
        return try {
            core.requestIntegrityToken(nonce, cloudProjectNumber)
        } catch (e: Exception) {
            throw Exception("Failed to request integrity token: ${e.message}", e)
        }
    }
}
