# Auroprint - Hardware-Backed Device Fingerprinting for Godot

**Auroprint** is a GDExtension Android plugin for Godot 4.2+ that provides cryptographically secure device fingerprinting using Android's hardware-backed keystore (TEE/StrongBox).

## What is Auroprint?

Auroprint generates a unique, unforgeable device fingerprint by combining:
- **Persistent Device ID**: Generated from hardware characteristics and MediaDRM
- **Cryptographic Signatures**: Signed using keys stored in Android's hardware security module
- **Attestation Chains**: Certificate chains proving the key is hardware-backed
- **Play Integrity API**: Optional integration for Google Play Integrity tokens

Unlike software-based fingerprinting methods that can be spoofed, Auroprint leverages the Trusted Execution Environment (TEE) or StrongBox security chip in modern Android devices to create tamper-proof device identities.

## Why Use Auroprint?

### Use Cases
- **Anti-Cheat Systems**: Detect and ban devices (not just accounts) used for cheating
- **Device Authentication**: Verify device identity for secure backend operations
- **Fraud Prevention**: Identify devices involved in fraudulent activities
- **License Enforcement**: Tie licenses to specific hardware
- **Analytics**: Track unique devices without relying on user accounts

### Key Features
- Hardware-backed cryptographic security (TEE/StrongBox)
- Unforgeable device signatures with attestation proof
- Persistent device IDs that survive app reinstalls
- Optional Google Play Integrity token support
- Zero permissions required (only INTERNET for Play Integrity)
- Works on physical devices with hardware keystore (Android 6.0+)

## How It Works

### Architecture

```
┌─────────────────────────────────────────────────┐
│           Godot Game (GDScript)                 │
│  auroprint.generate_auroprint()                 │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│       C++ GDExtension Layer                     │
│  - Auroprint singleton                          │
│  - Signal-based async API                       │
│  - JNI bridge to Kotlin                         │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│       Kotlin/Java Layer                         │
│  - AuroprintCore (crypto logic)                 │
│  - AuroprintPluginSync (JNI wrapper)            │
└────────────────┬────────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────────┐
│    Android Keystore (Hardware)                  │
│  - TEE or StrongBox                             │
│  - RSA 2048 key generation                      │
│  - SHA256withRSA signatures                     │
│  - Attestation certificate chains               │
└─────────────────────────────────────────────────┘
```

### Device ID Generation

The device ID is a cryptographic hash combining:
1. **MediaDRM Widevine ID** (persistent across factory resets on most devices)
2. **Hardware Properties**: Board, bootloader, brand, device, manufacturer, model, SoC
3. **Display Metrics**: Screen resolution and DPI

### Cryptographic Security

1. **Key Generation**: 2048-bit RSA key pair generated in Android Keystore
   - On devices with StrongBox (Pixel 3+, S9+): Uses dedicated security chip
   - On devices with TEE (Android 6.0+): Uses Trusted Execution Environment
   - Keys **never leave** the secure hardware

2. **Attestation**: Certificate chains prove the key is hardware-backed
   - Chain contains 3-5 certificates from device → manufacturer → Google
   - Verifiable by your backend to confirm hardware security

3. **Signing**: Payload is signed using SHA256withRSA
   - Signature can only be created by the specific device
   - Payload includes device ID, timestamp, and random nonce

## Integration Guide

### Prerequisites

- Godot 4.2 or higher
- Android 6.0+ target device with hardware keystore
- Physical device (not emulator) for hardware-backed features

### Step 1: Build the Plugin

```bash
# Clone or download this repository
cd auroprint-godot-plugin

# Initialize godot-cpp submodule
git submodule update --init

# Build godot-cpp for Android
cd godot-cpp
scons platform=android target=template_debug
scons platform=android target=template_release
cd ..

# Build the plugin
./gradlew clean assemble
```

The built plugin will be in `plugin/demo/addons/auroprint/`

### Step 2: Copy Plugin to Your Project

Copy the entire `plugin/demo/addons/auroprint/` directory to your Godot project's `addons/` folder:

```
your-godot-project/
├── addons/
│   └── auroprint/
│       ├── bin/
│       │   ├── debug/
│       │   │   └── auroprint-debug.aar
│       │   └── release/
│       │       └── auroprint-release.aar
│       ├── auroprint.gdextension
│       └── plugin.cfg
```

### Step 3: Enable the Plugin

1. Open your project in Godot
2. Go to `Project → Project Settings → Plugins`
3. Enable the **Auroprint** plugin

### Step 4: Configure Android Export

1. Install Android build template: `Project → Install Android Build Template`
2. Open `Project → Export → Android`
3. In the export preset, ensure:
   - `Gradle Build/Use Gradle Build` is **enabled**
   - Under `Plugins`, **Auroprint** is **checked**
   - `GDExtension/Export All` is **enabled**
4. For Play Integrity support, add `INTERNET` permission in export settings

### Step 5: Add Play Integrity Dependency (Required for Play Integrity API)

If you plan to use the Play Integrity token feature, you must add the Play Integrity dependency to your project's Android build configuration:

1. Open `android/build/build.gradle` in your Godot project
2. Find the `dependencies` section
3. Add the following line:

```gradle
dependencies {
    // ... existing dependencies ...

    // Required for Auroprint Play Integrity API support
    implementation 'com.google.android.play:integrity:1.3.0'
}
```

**Note**: This step is only required if you use `request_integrity_token()`. Device fingerprinting works without this dependency.

### Step 6: Use in GDScript

```gdscript
extends Node

var auroprint: Object

func _ready():
    # Get the Auroprint singleton
    if Engine.has_singleton("Auroprint"):
        auroprint = Engine.get_singleton("Auroprint")
        print("Auroprint plugin loaded successfully")

        # Connect signals
        auroprint.auroprint_generated.connect(_on_auroprint_generated)
        auroprint.auroprint_error.connect(_on_auroprint_error)

        # Generate auroprint
        auroprint.generate_auroprint()
    else:
        push_error("Auroprint plugin not available")

func _on_auroprint_generated(result: AuroprintResult):
    print("Device ID: ", result.get_device_id())
    print("Payload: ", result.get_payload())
    print("Signature: ", result.get_signature())
    print("Public Key: ", result.get_public_key())
    print("Attestation Chain Length: ", result.get_attestation_chain().size())
    print("Timestamp: ", result.get_timestamp())
    print("Nonce: ", result.get_nonce())
    print("Hardware Backed: ", result.get_is_hardware_backed())

    # Send to your backend for verification
    send_to_backend(result.to_dictionary())

func _on_auroprint_error(error_message: String):
    push_error("Auroprint error: " + error_message)

func send_to_backend(data: Dictionary):
    # Example: Send to your server
    var http = HTTPRequest.new()
    add_child(http)
    http.request("https://your-backend.com/verify-device",
                 ["Content-Type: application/json"],
                 HTTPClient.METHOD_POST,
                 JSON.stringify(data))
```

## API Reference

### Singleton: `Auroprint`

Access via: `Engine.get_singleton("Auroprint")`

All methods are **asynchronous** and return results via signals.

---

#### `generate_auroprint()`

Generates a complete device fingerprint with signature and attestation.

**Parameters**: None

**Returns**: Emits signal `auroprint_generated` with `AuroprintResult` on success, or `auroprint_error` with error message on failure.

**Example**:
```gdscript
auroprint.auroprint_generated.connect(_on_generated)
auroprint.auroprint_error.connect(_on_error)
auroprint.generate_auroprint()
```

---

#### `is_hardware_backed_available()`

Checks if the device supports hardware-backed keystore.

**Parameters**: None

**Returns**: Emits signal `hardware_backed_result` with `bool` value.

**Example**:
```gdscript
auroprint.hardware_backed_result.connect(_on_hw_check)
auroprint.is_hardware_backed_available()

func _on_hw_check(available: bool):
    if available:
        print("Hardware-backed keystore is available")
    else:
        print("Device does not support hardware-backed keystore")
```

---

#### `reset_key()`

Deletes the current signing key. The next call to `generate_auroprint()` will create a new key.

**Parameters**: None

**Returns**: Emits signal `key_reset_complete` on success, or `auroprint_error` on failure.

**Example**:
```gdscript
auroprint.key_reset_complete.connect(_on_reset)
auroprint.reset_key()

func _on_reset():
    print("Signing key has been reset")
```

**Use Cases**:
- Testing different device fingerprints
- Allowing users to "unlink" their device
- Recovering from corrupted key states

---

#### `request_integrity_token(nonce: String, cloud_project_number: int)`

Requests a Google Play Integrity token. Requires `INTERNET` permission.

**Parameters**:
- `nonce` (String): Base64-encoded nonce for the integrity request
- `cloud_project_number` (int): Your Google Cloud project number (optional, pass 0 to omit)

**Returns**: Emits signal `integrity_token_received` with token String on success, or `integrity_token_error` with error message on failure.

**Example**:
```gdscript
auroprint.integrity_token_received.connect(_on_token)
auroprint.integrity_token_error.connect(_on_token_error)

var nonce = "your-base64-nonce"
var project_number = 123456789  # Your Google Cloud project number
auroprint.request_integrity_token(nonce, project_number)

func _on_token(token: String):
    print("Integrity Token: ", token)
    # Send to your backend for verification
```

**Requirements**:
- App must be published on Google Play (at least internal testing track)
- Play Integrity API must be enabled in Google Cloud Console
- Device must have Google Play Services

---

### Signals

| Signal | Parameters | Description |
|--------|-----------|-------------|
| `auroprint_generated` | `AuroprintResult` | Emitted when fingerprint generation succeeds |
| `auroprint_error` | `String` error_message | Emitted when any operation fails |
| `hardware_backed_result` | `bool` available | Emitted with hardware availability status |
| `key_reset_complete` | None | Emitted when key reset succeeds |
| `integrity_token_received` | `String` token | Emitted when Play Integrity token is received |
| `integrity_token_error` | `String` error_message | Emitted when integrity token request fails |

---

### Class: `AuroprintResult`

Result object containing device fingerprint data. Inherits from `RefCounted`.

#### Properties

| Property | Type | Description |
|----------|------|-------------|
| `device_id` | String | Unique device identifier (SHA-256 hash) |
| `payload` | String | JSON payload containing device ID, timestamp, and nonce |
| `signature` | String | Base64-encoded RSA signature of the payload |
| `public_key` | String | PEM-encoded public key certificate |
| `attestation_chain` | Array[String] | Array of PEM-encoded attestation certificates |
| `timestamp` | int | Unix timestamp (seconds) when fingerprint was generated |
| `nonce` | String | Random nonce included in the payload |
| `is_hardware_backed` | bool | Whether the signing key is hardware-backed |

#### Methods

##### Getters

```gdscript
get_device_id() -> String
get_payload() -> String
get_signature() -> String
get_public_key() -> String
get_attestation_chain() -> Array
get_timestamp() -> int
get_nonce() -> String
get_is_hardware_backed() -> bool
```

##### `to_dictionary() -> Dictionary`

Converts the result to a Dictionary for easy JSON serialization.

**Example**:
```gdscript
func _on_auroprint_generated(result: AuroprintResult):
    var data = result.to_dictionary()
    # {
    #   "deviceId": "a1b2c3...",
    #   "payload": "{\"did\":\"a1b2c3...\",\"ts\":1704844800,\"nonce\":\"...\"}",
    #   "signature": "Base64String...",
    #   "publicKey": "-----BEGIN CERTIFICATE-----\n...",
    #   "attestationChain": ["-----BEGIN CERTIFICATE-----\n...", ...],
    #   "timestamp": 1704844800,
    #   "nonce": "32charhexstring",
    #   "isHardwareBacked": true
    # }

    var json = JSON.stringify(data)
    send_to_backend(json)
```

---

## Backend Verification

To verify an auroprint on your backend:

1. **Verify Signature**:
   ```python
   from cryptography.hazmat.primitives import hashes, serialization
   from cryptography.hazmat.primitives.asymmetric import padding
   import base64

   # Load public key from PEM
   public_key = serialization.load_pem_public_key(
       result['publicKey'].encode()
   )

   # Verify signature
   signature_bytes = base64.b64decode(result['signature'])
   payload_bytes = result['payload'].encode('utf-8')

   public_key.verify(
       signature_bytes,
       payload_bytes,
       padding.PKCS1v15(),
       hashes.SHA256()
   )
   # If no exception, signature is valid
   ```

2. **Verify Attestation Chain** (Optional but Recommended):
   - Parse the certificate chain
   - Verify each certificate is signed by the next
   - Check root certificate is from Google or device manufacturer
   - Verify attestation extension proves hardware backing

3. **Store Device ID**:
   - Save `deviceId` in your database
   - Associate with user account if applicable
   - Track for anti-cheat, fraud detection, etc.

4. **Check Timestamp & Nonce**:
   - Verify timestamp is recent (prevent replay attacks)
   - Ensure nonce hasn't been used before

---

## Troubleshooting

### "Auroprint plugin not available"

**Cause**: Plugin not loaded by Godot

**Solutions**:
1. Ensure plugin is enabled in `Project → Project Settings → Plugins`
2. Verify `auroprint.gdextension` exists in `addons/auroprint/`
3. Check export preset has **Auroprint** plugin checked
4. Confirm `GDExtension/Export All` is enabled in export settings
5. Clear Godot export cache: delete `android/build` and `.godot/exported`

### Attestation chain length is 0

**Cause**: Using desktop stub instead of Android implementation

**Solutions**:
1. Ensure you're running on a **physical Android device** (not desktop or emulator)
2. Verify `ANDROID_ENABLED` flag is defined in CMakeLists.txt
3. Rebuild plugin with `./gradlew clean assemble`
4. Copy fresh AAR files to your project
5. Clear build caches before testing

### "Hardware-backed keystore not available"

**Cause**: Device doesn't support TEE/StrongBox

**Solutions**:
- Requires Android 6.0+ with hardware-backed keystore
- Emulators don't support hardware backing
- Very old devices may not have TEE
- Fingerprint will still work, but attestation chain may be shorter/weaker

### NoClassDefFoundError: IntegrityManagerFactory

**Error**: `java.lang.NoClassDefFoundError: Failed resolution of: Lcom/google/android/play/core/integrity/IntegrityManagerFactory;`

**Cause**: Play Integrity API dependency not included in your Godot project's APK

**Solution**: Add the dependency to your project's `android/build/build.gradle`:
```gradle
dependencies {
    implementation 'com.google.android.play:integrity:1.3.0'
}
```

See **Step 5** in the integration guide above for full details.

### Play Integrity token errors

**Common Issues**:
1. **App not published**: Must be on at least internal testing track
2. **API not enabled**: Enable Play Integrity API in Google Cloud Console
3. **No Google Play Services**: Device must have Play Services installed
4. **Missing INTERNET permission**: Add in Android export settings

---

## Security Considerations

### What Auroprint Provides
- Cryptographically unforgeable device signatures
- Hardware-backed key storage (keys never exposed)
- Attestation proof of hardware security
- Persistent device identification

### What Auroprint Does NOT Provide
- **Anti-root/jailbreak detection**: Rooted devices can still generate valid fingerprints
- **Emulator detection**: Use Play Integrity API for that
- **Network security**: Always use HTTPS when transmitting fingerprints
- **User privacy protection**: Fingerprints are persistent across app reinstalls

### Privacy Compliance

Device fingerprinting may be subject to privacy regulations (GDPR, CCPA, etc.):
- Inform users about device fingerprinting in your privacy policy
- Consider requiring user consent before fingerprinting
- Provide opt-out mechanisms if required by your jurisdiction
- Don't use fingerprints for purposes beyond stated use cases

---

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Android 6.0+ (Physical) | ✅ Fully Supported | Hardware-backed keystore with TEE |
| Android 9.0+ (StrongBox) | ✅ Enhanced Security | Dedicated security chip |
| Android Emulator | ⚠️ Limited | No hardware backing, shorter attestation chains |
| iOS | ❌ Not Supported | Future implementation planned |
| Desktop (Windows/Mac/Linux) | ⚠️ Stub Only | Returns mock data for testing |

---

## License

[Add your license here]

---

## Credits

- **Author**: [Your Name/Organization]
- **Based on**: Godot GDExtension Android Plugin Template by Fredia Huya-Kouadio
- **Android Keystore**: Uses Android's hardware security module APIs
- **Play Integrity**: Google Play Integrity API integration

---

## Support

For issues, questions, or contributions:
- Report bugs at [repository issue tracker]
- Documentation: This README
- Godot Forums: [link if applicable]
