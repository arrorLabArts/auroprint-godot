#include <jni.h>
#include <android/log.h>

#include <godot_cpp/variant/utility_functions.hpp>

#include "utils.h"

#define LOG_TAG "AuroprintJNI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#undef JNI_PACKAGE_NAME
#define JNI_PACKAGE_NAME com_madeofcode_auroprint

#undef JNI_CLASS_NAME
#define JNI_CLASS_NAME AuroprintGodotPlugin

// Global JavaVM pointer - shared with auroprint_android.cpp
JavaVM* g_jvm = nullptr;

extern "C" {
    /**
     * Called from AuroprintGodotPlugin constructor to initialize JNI environment
     * This captures the JavaVM pointer so it can be used later by the GDExtension code
     */
    JNIEXPORT void JNICALL JNI_METHOD(nativeInitialize)(JNIEnv *env, jobject thiz) {
        if (g_jvm == nullptr) {
            jint result = env->GetJavaVM(&g_jvm);
            if (result == JNI_OK && g_jvm != nullptr) {
                LOGD("✓ Native JNI initialized successfully, JavaVM captured!");
                // NOTE: Cannot call godot::UtilityFunctions::print() here because
                // godot-cpp is not initialized yet at this point
            } else {
                LOGE("✗ Failed to get JavaVM in nativeInitialize");
            }
        } else {
            LOGD("JavaVM already initialized");
        }
    }
};
