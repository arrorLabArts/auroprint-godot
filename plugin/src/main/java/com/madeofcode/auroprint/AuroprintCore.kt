package com.madeofcode.auroprint

import android.content.Context
import android.media.MediaDrm
import android.os.Build
import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.security.keystore.StrongBoxUnavailableException
import android.util.Base64
import com.google.android.play.core.integrity.IntegrityManagerFactory
import com.google.android.play.core.integrity.IntegrityTokenRequest
import org.json.JSONObject
import java.security.KeyPairGenerator
import java.security.KeyStore
import java.security.PrivateKey
import java.security.Signature
import java.security.cert.Certificate
import java.security.cert.X509Certificate
import java.util.UUID

/**
 * Core Auroprint cryptographic functionality for Godot.
 * This is a standalone implementation without Flutter dependencies.
 */
class AuroprintCore(private val context: Context) {

    companion object {
        private const val KEY_ALIAS = "auroprint_signing_key"
        private const val ANDROID_KEYSTORE = "AndroidKeyStore"

        // Widevine UUID for MediaDRM
        private val WIDEVINE_UUID = UUID(-0x121074568629b532L, -0x5c37d8232ae2de13L)
    }

    /**
     * Generate a complete auroprint fingerprint
     * Returns a map with all auroprint data
     */
    fun generateAuroprint(): Map<String, Any> {
        // Ensure key exists
        ensureKeyExists()

        // Get persistent device ID
        val deviceId = getDeviceId()

        // Generate timestamp and nonce
        val timestamp = System.currentTimeMillis() / 1000
        val nonce = UUID.randomUUID().toString().replace("-", "")

        // Create payload
        val payloadJson = JSONObject().apply {
            put("did", deviceId)
            put("ts", timestamp)
            put("nonce", nonce)
        }
        val payload = payloadJson.toString()

        // Sign the payload
        val signature = signPayload(payload)

        // Get public key and attestation chain
        val keyStore = KeyStore.getInstance(ANDROID_KEYSTORE)
        keyStore.load(null)
        val certificateChain = keyStore.getCertificateChain(KEY_ALIAS)

        val publicKeyPem = certificateToPem(certificateChain[0])
        val attestationChain = certificateChain.map { certificateToPem(it) }

        // Check if hardware-backed
        val isHardwareBacked = isKeyHardwareBacked()

        return hashMapOf(
            "deviceId" to deviceId,
            "payload" to payload,
            "signature" to signature,
            "publicKey" to publicKeyPem,
            "attestationChain" to attestationChain,
            "timestamp" to timestamp,
            "nonce" to nonce,
            "isHardwareBacked" to isHardwareBacked
        )
    }

    /**
     * Check if hardware-backed keystore is available
     */
    fun isHardwareBackedAvailable(): Boolean {
        return Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
    }

    /**
     * Reset the signing key (forces regeneration on next use)
     */
    fun resetKey() {
        val keyStore = KeyStore.getInstance(ANDROID_KEYSTORE)
        keyStore.load(null)
        if (keyStore.containsAlias(KEY_ALIAS)) {
            keyStore.deleteEntry(KEY_ALIAS)
        }
    }

    /**
     * Request a Play Integrity token
     * Returns the token string
     */
    fun requestIntegrityToken(nonce: String, cloudProjectNumber: Long): String {
        val integrityManager = IntegrityManagerFactory.create(context)

        // Build the request
        val requestBuilder = IntegrityTokenRequest.builder()
            .setNonce(nonce)

        // Add cloud project number if provided
        if (cloudProjectNumber > 0) {
            requestBuilder.setCloudProjectNumber(cloudProjectNumber)
        }

        // This is synchronous blocking - will be called from worker thread
        val task = integrityManager.requestIntegrityToken(requestBuilder.build())

        // Wait for result (this blocks, so must be called from background thread)
        while (!task.isComplete) {
            Thread.sleep(50)
        }

        if (task.isSuccessful) {
            return task.result.token()
        } else {
            throw task.exception ?: Exception("Unknown integrity token error")
        }
    }

    // ========== Private Helper Methods ==========

    private fun ensureKeyExists() {
        val keyStore = KeyStore.getInstance(ANDROID_KEYSTORE)
        keyStore.load(null)

        if (!keyStore.containsAlias(KEY_ALIAS)) {
            generateSigningKey()
        }
    }

    private fun generateSigningKey() {
        val keyPairGenerator = KeyPairGenerator.getInstance(
            KeyProperties.KEY_ALGORITHM_RSA,
            ANDROID_KEYSTORE
        )

        val builder = KeyGenParameterSpec.Builder(
            KEY_ALIAS,
            KeyProperties.PURPOSE_SIGN or KeyProperties.PURPOSE_VERIFY
        )
            .setDigests(KeyProperties.DIGEST_SHA256)
            .setSignaturePaddings(KeyProperties.SIGNATURE_PADDING_RSA_PKCS1)
            .setKeySize(2048)
            .setUserAuthenticationRequired(false)

        // Request attestation if available (API 24+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            val challenge = "auroprint_attestation_${System.currentTimeMillis()}".toByteArray()
            builder.setAttestationChallenge(challenge)
        }

        // Try StrongBox first (API 28+), fall back to TEE if unavailable
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            try {
                builder.setIsStrongBoxBacked(true)
                keyPairGenerator.initialize(builder.build())
                keyPairGenerator.generateKeyPair()
                return
            } catch (e: StrongBoxUnavailableException) {
                // StrongBox not available, fall back to TEE
                val teeBuilder = KeyGenParameterSpec.Builder(
                    KEY_ALIAS,
                    KeyProperties.PURPOSE_SIGN or KeyProperties.PURPOSE_VERIFY
                )
                    .setDigests(KeyProperties.DIGEST_SHA256)
                    .setSignaturePaddings(KeyProperties.SIGNATURE_PADDING_RSA_PKCS1)
                    .setKeySize(2048)
                    .setUserAuthenticationRequired(false)

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                    val challenge = "auroprint_attestation_${System.currentTimeMillis()}".toByteArray()
                    teeBuilder.setAttestationChallenge(challenge)
                }

                keyPairGenerator.initialize(teeBuilder.build())
                keyPairGenerator.generateKeyPair()
                return
            }
        }

        keyPairGenerator.initialize(builder.build())
        keyPairGenerator.generateKeyPair()
    }

    private fun signPayload(payload: String): String {
        val keyStore = KeyStore.getInstance(ANDROID_KEYSTORE)
        keyStore.load(null)
        val privateKey = keyStore.getKey(KEY_ALIAS, null) as PrivateKey

        val signature = Signature.getInstance("SHA256withRSA")
        signature.initSign(privateKey)
        signature.update(payload.toByteArray(Charsets.UTF_8))
        val signatureBytes = signature.sign()

        return Base64.encodeToString(signatureBytes, Base64.NO_WRAP)
    }

    private fun getDeviceId(): String {
        val components = mutableListOf<String>()

        // 1. MediaDRM ID
        try {
            val mediaDrmId = getMediaDrmId()
            if (mediaDrmId.isNotEmpty()) {
                components.add(mediaDrmId)
            }
        } catch (e: Exception) {
            // MediaDRM not available
        }

        // 2. Hardware properties
        components.addAll(listOf(
            Build.BOARD,
            Build.BOOTLOADER,
            Build.BRAND,
            Build.DEVICE,
            Build.HARDWARE,
            Build.MANUFACTURER,
            Build.MODEL,
            Build.PRODUCT
        ))

        // Add SoC info for API 31+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            components.add(Build.SOC_MANUFACTURER)
            components.add(Build.SOC_MODEL)
        }

        // 3. Display metrics
        try {
            val displayMetrics = context.resources.displayMetrics
            components.add("${displayMetrics.widthPixels}x${displayMetrics.heightPixels}")
            components.add("${displayMetrics.densityDpi}")
        } catch (e: Exception) {
            // Display info not available
        }

        // Combine all components into a single hash
        val combined = components.joinToString("|")
        return hashString(combined)
    }

    private fun getMediaDrmId(): String {
        val mediaDrm = MediaDrm(WIDEVINE_UUID)
        try {
            val deviceId = mediaDrm.getPropertyByteArray(MediaDrm.PROPERTY_DEVICE_UNIQUE_ID)
            return Base64.encodeToString(deviceId, Base64.NO_WRAP)
        } finally {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                mediaDrm.close()
            } else {
                @Suppress("DEPRECATION")
                mediaDrm.release()
            }
        }
    }

    private fun hashString(input: String): String {
        val bytes = java.security.MessageDigest.getInstance("SHA-256")
            .digest(input.toByteArray(Charsets.UTF_8))
        return bytes.joinToString("") { "%02x".format(it) }
    }

    private fun certificateToPem(certificate: Certificate): String {
        val base64 = Base64.encodeToString(certificate.encoded, Base64.NO_WRAP)
        return "-----BEGIN CERTIFICATE-----\n${base64.chunked(64).joinToString("\n")}\n-----END CERTIFICATE-----"
    }

    private fun isKeyHardwareBacked(): Boolean {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return false
        }

        try {
            val keyStore = KeyStore.getInstance(ANDROID_KEYSTORE)
            keyStore.load(null)
            val certificate = keyStore.getCertificate(KEY_ALIAS) as? X509Certificate
                ?: return false

            return certificate.publicKey != null
        } catch (e: Exception) {
            return false
        }
    }
}
