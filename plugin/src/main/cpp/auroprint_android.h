#ifndef AUROPRINT_ANDROID_H
#define AUROPRINT_ANDROID_H

#ifdef ANDROID_ENABLED

#include "auroprint.h"
#include "auroprint_result.h"

#include <jni.h>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

/**
 * Android-specific implementation of Auroprint
 * Uses JNI to call the existing Kotlin AuroprintPlugin
 */
class AuroprintAndroid {
public:
    // JNI helper methods
    static JNIEnv* get_jni_env();
    static jobject get_activity();

    // Type conversion helpers
    static jstring godot_string_to_jstring(JNIEnv* env, const String& str);
    static String jstring_to_godot_string(JNIEnv* env, jstring jstr);
    static Ref<AuroprintResult> jobject_to_auroprint_result(JNIEnv* env, jobject result_obj);
    static Array jobject_list_to_array(JNIEnv* env, jobject list);

    // Kotlin plugin interface
    static jobject get_kotlin_plugin_instance(JNIEnv* env);
};

#endif // ANDROID_ENABLED

#endif // AUROPRINT_ANDROID_H
