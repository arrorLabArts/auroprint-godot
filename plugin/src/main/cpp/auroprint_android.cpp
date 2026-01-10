#ifdef ANDROID_ENABLED

#include "auroprint_android.h"
#include "auroprint.h"
#include "auroprint_result.h"
#include <android/log.h>
#include <godot_cpp/variant/utility_functions.hpp>

#define LOG_TAG "AuroprintAndroid"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Reference to JavaVM initialized in plugin_jni.cpp
extern JavaVM* g_jvm;

// JNI Environment access
JNIEnv* AuroprintAndroid::get_jni_env() {
    if (!g_jvm) {
        LOGE("JavaVM not initialized - plugin may not have loaded properly");
        return nullptr;
    }

    JNIEnv* env = nullptr;
    jint result = g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (result == JNI_EDETACHED) {
        result = g_jvm->AttachCurrentThread(&env, nullptr);
        if (result != JNI_OK) {
            LOGE("Failed to attach thread to JVM");
            return nullptr;
        }
        LOGD("Thread attached to JVM");
    }

    return env;
}

jobject AuroprintAndroid::get_activity() {
    JNIEnv* env = get_jni_env();
    if (!env) {
        LOGE("Cannot get JNI environment");
        return nullptr;
    }

    // Get the AuroprintGodotPlugin singleton instance
    jclass plugin_class = env->FindClass("com/madeofcode/auroprint/AuroprintGodotPlugin");
    if (!plugin_class) {
        LOGE("Failed to find AuroprintGodotPlugin class");
        return nullptr;
    }

    // Get the getInstance() static method
    jmethodID get_instance = env->GetStaticMethodID(plugin_class, "getInstance",
        "()Lcom/madeofcode/auroprint/AuroprintGodotPlugin;");
    if (!get_instance) {
        LOGE("Failed to find getInstance method");
        env->DeleteLocalRef(plugin_class);
        return nullptr;
    }

    // Get the plugin instance
    jobject plugin_instance = env->CallStaticObjectMethod(plugin_class, get_instance);
    if (!plugin_instance) {
        LOGE("Plugin instance is null - plugin may not be loaded");
        env->DeleteLocalRef(plugin_class);
        return nullptr;
    }

    // Get the getAndroidActivity() method
    jmethodID get_activity = env->GetMethodID(plugin_class, "getAndroidActivity",
        "()Landroid/app/Activity;");
    if (!get_activity) {
        LOGE("Failed to find getAndroidActivity method");
        env->DeleteLocalRef(plugin_instance);
        env->DeleteLocalRef(plugin_class);
        return nullptr;
    }

    // Call getAndroidActivity() on the plugin instance
    jobject activity = env->CallObjectMethod(plugin_instance, get_activity);

    env->DeleteLocalRef(plugin_instance);
    env->DeleteLocalRef(plugin_class);

    if (!activity) {
        LOGE("getActivity returned null");
    }

    return activity;
}

// Type conversion helpers
jstring AuroprintAndroid::godot_string_to_jstring(JNIEnv* env, const String& str) {
    return env->NewStringUTF(str.utf8().get_data());
}

String AuroprintAndroid::jstring_to_godot_string(JNIEnv* env, jstring jstr) {
    if (!jstr) return "";

    const char* utf_chars = env->GetStringUTFChars(jstr, nullptr);
    String result(utf_chars);
    env->ReleaseStringUTFChars(jstr, utf_chars);

    return result;
}

Array AuroprintAndroid::jobject_list_to_array(JNIEnv* env, jobject list) {
    Array result;

    if (!list) return result;

    // Get List class and size() method
    jclass list_class = env->FindClass("java/util/List");
    jmethodID size_method = env->GetMethodID(list_class, "size", "()I");
    jmethodID get_method = env->GetMethodID(list_class, "get", "(I)Ljava/lang/Object;");

    jint size = env->CallIntMethod(list, size_method);

    for (jint i = 0; i < size; i++) {
        jobject item = env->CallObjectMethod(list, get_method, i);
        if (item) {
            jstring jstr = static_cast<jstring>(item);
            result.append(jstring_to_godot_string(env, jstr));
            env->DeleteLocalRef(item);
        }
    }

    env->DeleteLocalRef(list_class);

    return result;
}

Ref<AuroprintResult> AuroprintAndroid::jobject_to_auroprint_result(JNIEnv* env, jobject result_obj) {
    Ref<AuroprintResult> result;
    result.instantiate();

    if (!result_obj) {
        LOGE("Received null result object");
        return result;
    }

    // Get HashMap class
    jclass map_class = env->FindClass("java/util/HashMap");
    jmethodID get_method = env->GetMethodID(map_class, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");

    // Helper lambda to get string value from map
    auto get_string = [&](const char* key) -> String {
        jstring key_str = env->NewStringUTF(key);
        jobject value = env->CallObjectMethod(result_obj, get_method, key_str);
        env->DeleteLocalRef(key_str);

        if (value) {
            String result = jstring_to_godot_string(env, static_cast<jstring>(value));
            env->DeleteLocalRef(value);
            return result;
        }
        return "";
    };

    // Helper to get long value from map
    auto get_long = [&](const char* key) -> int64_t {
        jstring key_str = env->NewStringUTF(key);
        jobject value = env->CallObjectMethod(result_obj, get_method, key_str);
        env->DeleteLocalRef(key_str);

        if (value) {
            jclass long_class = env->FindClass("java/lang/Long");
            jmethodID long_value = env->GetMethodID(long_class, "longValue", "()J");
            jlong result = env->CallLongMethod(value, long_value);
            env->DeleteLocalRef(long_class);
            env->DeleteLocalRef(value);
            return static_cast<int64_t>(result);
        }
        return 0;
    };

    // Helper to get boolean value from map
    auto get_boolean = [&](const char* key) -> bool {
        jstring key_str = env->NewStringUTF(key);
        jobject value = env->CallObjectMethod(result_obj, get_method, key_str);
        env->DeleteLocalRef(key_str);

        if (value) {
            jclass bool_class = env->FindClass("java/lang/Boolean");
            jmethodID bool_value = env->GetMethodID(bool_class, "booleanValue", "()Z");
            jboolean result = env->CallBooleanMethod(value, bool_value);
            env->DeleteLocalRef(bool_class);
            env->DeleteLocalRef(value);
            return result == JNI_TRUE;
        }
        return false;
    };

    // Helper to get list value from map
    auto get_list = [&](const char* key) -> Array {
        jstring key_str = env->NewStringUTF(key);
        jobject value = env->CallObjectMethod(result_obj, get_method, key_str);
        env->DeleteLocalRef(key_str);

        if (value) {
            Array result = jobject_list_to_array(env, value);
            env->DeleteLocalRef(value);
            return result;
        }
        return Array();
    };

    // Extract all fields from the HashMap
    result->set_device_id(get_string("deviceId"));
    result->set_payload(get_string("payload"));
    result->set_signature(get_string("signature"));
    result->set_public_key(get_string("publicKey"));
    result->set_attestation_chain(get_list("attestationChain"));
    result->set_timestamp(get_long("timestamp"));
    result->set_nonce(get_string("nonce"));
    result->set_is_hardware_backed(get_boolean("isHardwareBacked"));

    env->DeleteLocalRef(map_class);

    return result;
}

jobject AuroprintAndroid::get_kotlin_plugin_instance(JNIEnv* env) {
    // Get the Kotlin AuroprintPluginSync class
    jclass plugin_class = env->FindClass("com/madeofcode/auroprint/AuroprintPluginSync");
    if (!plugin_class) {
        LOGE("Failed to find Kotlin AuroprintPluginSync class");
        return nullptr;
    }

    // Get the Android context
    jobject context = get_activity();
    if (!context) {
        LOGE("Failed to get Android context");
        env->DeleteLocalRef(plugin_class);
        return nullptr;
    }

    // Get the constructor that takes Context
    jmethodID constructor = env->GetMethodID(plugin_class, "<init>", "(Landroid/content/Context;)V");
    if (!constructor) {
        LOGE("Failed to find AuroprintPluginSync constructor");
        env->DeleteLocalRef(plugin_class);
        env->DeleteLocalRef(context);
        return nullptr;
    }

    // Create instance with context
    jobject plugin_instance = env->NewObject(plugin_class, constructor, context);

    env->DeleteLocalRef(plugin_class);
    env->DeleteLocalRef(context);

    return plugin_instance;
}

// Platform-specific implementations for the Auroprint class
Ref<AuroprintResult> Auroprint::_platform_generate_auroprint() {
    JNIEnv* env = AuroprintAndroid::get_jni_env();
    if (!env) {
        throw std::runtime_error("Failed to get JNI environment");
    }

    LOGD("Generating auroprint via JNI...");

    // Get Kotlin plugin instance
    jobject plugin = AuroprintAndroid::get_kotlin_plugin_instance(env);
    if (!plugin) {
        throw std::runtime_error("Failed to create Kotlin plugin instance");
    }

    // Get the plugin class
    jclass plugin_class = env->FindClass("com/madeofcode/auroprint/AuroprintPluginSync");

    // Get the generateAuroprintSync method (we'll create a synchronous version)
    // Since the Kotlin version is async, we need to wait for the result
    // For now, let's use a simpler approach and create a blocking wrapper in Kotlin

    // Get the generateAuroprint method that returns HashMap directly
    jmethodID generate_method = env->GetMethodID(plugin_class, "generateAuroprintSync",
        "()Ljava/util/HashMap;");

    if (!generate_method) {
        LOGE("Failed to find generateAuroprintSync method - you may need to add this to the Kotlin plugin");
        env->DeleteLocalRef(plugin_class);
        env->DeleteLocalRef(plugin);
        throw std::runtime_error("generateAuroprintSync method not found in Kotlin plugin");
    }

    // Call the method
    jobject result_obj = env->CallObjectMethod(plugin, generate_method);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(plugin_class);
        env->DeleteLocalRef(plugin);
        throw std::runtime_error("Exception occurred while generating auroprint");
    }

    // Convert to AuroprintResult
    Ref<AuroprintResult> result = AuroprintAndroid::jobject_to_auroprint_result(env, result_obj);

    // Cleanup
    env->DeleteLocalRef(result_obj);
    env->DeleteLocalRef(plugin_class);
    env->DeleteLocalRef(plugin);

    LOGD("Auroprint generated successfully");

    return result;
}

bool Auroprint::_platform_is_hardware_backed_available() {
    JNIEnv* env = AuroprintAndroid::get_jni_env();
    if (!env) {
        throw std::runtime_error("Failed to get JNI environment");
    }

    jobject plugin = AuroprintAndroid::get_kotlin_plugin_instance(env);
    if (!plugin) {
        throw std::runtime_error("Failed to create Kotlin plugin instance");
    }

    jclass plugin_class = env->FindClass("com/madeofcode/auroprint/AuroprintPluginSync");
    jmethodID method = env->GetMethodID(plugin_class, "isHardwareBackedAvailableSync", "()Z");

    if (!method) {
        LOGE("Failed to find isHardwareBackedAvailableSync method");
        env->DeleteLocalRef(plugin_class);
        env->DeleteLocalRef(plugin);
        throw std::runtime_error("isHardwareBackedAvailableSync method not found");
    }

    jboolean available = env->CallBooleanMethod(plugin, method);

    env->DeleteLocalRef(plugin_class);
    env->DeleteLocalRef(plugin);

    return available == JNI_TRUE;
}

void Auroprint::_platform_reset_key() {
    JNIEnv* env = AuroprintAndroid::get_jni_env();
    if (!env) {
        throw std::runtime_error("Failed to get JNI environment");
    }

    jobject plugin = AuroprintAndroid::get_kotlin_plugin_instance(env);
    if (!plugin) {
        throw std::runtime_error("Failed to create Kotlin plugin instance");
    }

    jclass plugin_class = env->FindClass("com/madeofcode/auroprint/AuroprintPluginSync");
    jmethodID method = env->GetMethodID(plugin_class, "resetKeySync", "()V");

    if (!method) {
        LOGE("Failed to find resetKeySync method");
        env->DeleteLocalRef(plugin_class);
        env->DeleteLocalRef(plugin);
        throw std::runtime_error("resetKeySync method not found");
    }

    env->CallVoidMethod(plugin, method);

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(plugin_class);
        env->DeleteLocalRef(plugin);
        throw std::runtime_error("Exception occurred while resetting key");
    }

    env->DeleteLocalRef(plugin_class);
    env->DeleteLocalRef(plugin);
}

String Auroprint::_platform_request_integrity_token(const String &nonce, int64_t cloud_project_number) {
    JNIEnv* env = AuroprintAndroid::get_jni_env();
    if (!env) {
        throw std::runtime_error("Failed to get JNI environment");
    }

    jobject plugin = AuroprintAndroid::get_kotlin_plugin_instance(env);
    if (!plugin) {
        throw std::runtime_error("Failed to create Kotlin plugin instance");
    }

    jclass plugin_class = env->FindClass("com/madeofcode/auroprint/AuroprintPluginSync");
    jmethodID method = env->GetMethodID(plugin_class, "requestIntegrityTokenSync",
        "(Ljava/lang/String;J)Ljava/lang/String;");

    if (!method) {
        LOGE("Failed to find requestIntegrityTokenSync method");
        env->DeleteLocalRef(plugin_class);
        env->DeleteLocalRef(plugin);
        throw std::runtime_error("requestIntegrityTokenSync method not found");
    }

    jstring jnonce = AuroprintAndroid::godot_string_to_jstring(env, nonce);
    jlong jcloud_num = static_cast<jlong>(cloud_project_number);

    jstring result_str = static_cast<jstring>(env->CallObjectMethod(plugin, method, jnonce, jcloud_num));

    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(jnonce);
        env->DeleteLocalRef(plugin_class);
        env->DeleteLocalRef(plugin);
        throw std::runtime_error("Exception occurred while requesting integrity token");
    }

    String result = AuroprintAndroid::jstring_to_godot_string(env, result_str);

    env->DeleteLocalRef(jnonce);
    env->DeleteLocalRef(result_str);
    env->DeleteLocalRef(plugin_class);
    env->DeleteLocalRef(plugin);

    return result;
}

#endif // ANDROID_ENABLED
