#include "auroprint.h"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

Auroprint *Auroprint::singleton = nullptr;

Auroprint::Auroprint() {
	ERR_FAIL_COND(singleton != nullptr);
	singleton = this;
}

Auroprint::~Auroprint() {
	if (singleton == this) {
		singleton = nullptr;
	}
}

Auroprint *Auroprint::get_singleton() {
	if (singleton == nullptr) {
		singleton = memnew(Auroprint);
	}
	return singleton;
}

void Auroprint::_bind_methods() {
	// Bind static singleton getter
	ClassDB::bind_static_method("Auroprint", D_METHOD("get_singleton"), &Auroprint::get_singleton);

	// Bind public methods
	ClassDB::bind_method(D_METHOD("generate_auroprint"), &Auroprint::generate_auroprint);
	ClassDB::bind_method(D_METHOD("is_hardware_backed_available"), &Auroprint::is_hardware_backed_available);
	ClassDB::bind_method(D_METHOD("reset_key"), &Auroprint::reset_key);
	ClassDB::bind_method(D_METHOD("request_integrity_token", "nonce", "cloud_project_number"), &Auroprint::request_integrity_token, DEFVAL(0));

	// Bind worker methods (for callable_mp)
	ClassDB::bind_method(D_METHOD("_generate_auroprint_worker"), &Auroprint::_generate_auroprint_worker);
	ClassDB::bind_method(D_METHOD("_is_hardware_backed_available_worker"), &Auroprint::_is_hardware_backed_available_worker);
	ClassDB::bind_method(D_METHOD("_reset_key_worker"), &Auroprint::_reset_key_worker);

	// Register signals
	ADD_SIGNAL(MethodInfo("auroprint_generated", PropertyInfo(Variant::OBJECT, "result", PROPERTY_HINT_RESOURCE_TYPE, "AuroprintResult")));
	ADD_SIGNAL(MethodInfo("auroprint_error", PropertyInfo(Variant::STRING, "error_message")));
	ADD_SIGNAL(MethodInfo("hardware_backed_result", PropertyInfo(Variant::BOOL, "available")));
	ADD_SIGNAL(MethodInfo("key_reset_complete"));
	ADD_SIGNAL(MethodInfo("integrity_token_received", PropertyInfo(Variant::STRING, "token")));
	ADD_SIGNAL(MethodInfo("integrity_token_error", PropertyInfo(Variant::STRING, "error_message")));
}

// Public API methods
void Auroprint::generate_auroprint() {
	// Call directly on main thread (which is attached to JVM on Android)
	// The Kotlin code is already fast enough (~1-2 seconds)
	_generate_auroprint_worker();
}

void Auroprint::is_hardware_backed_available() {
	// Call directly on main thread
	_is_hardware_backed_available_worker();
}

void Auroprint::reset_key() {
	// Call directly on main thread
	_reset_key_worker();
}

void Auroprint::request_integrity_token(const String &nonce, int64_t cloud_project_number) {
	// Call directly on main thread
	_request_integrity_token_worker(nonce, cloud_project_number);
}

// Worker methods (run on background thread)
void Auroprint::_generate_auroprint_worker() {
	try {
		Ref<AuroprintResult> result = _platform_generate_auroprint();

		// Emit signal on main thread
		call_deferred("emit_signal", "auroprint_generated", result);
	} catch (const std::exception &e) {
		String error_msg = String("Failed to generate auroprint: ") + e.what();
		call_deferred("emit_signal", "auroprint_error", error_msg);
	} catch (...) {
		call_deferred("emit_signal", "auroprint_error", "Unknown error occurred while generating auroprint");
	}
}

void Auroprint::_is_hardware_backed_available_worker() {
	try {
		bool available = _platform_is_hardware_backed_available();
		call_deferred("emit_signal", "hardware_backed_result", available);
	} catch (const std::exception &e) {
		String error_msg = String("Failed to check hardware support: ") + e.what();
		call_deferred("emit_signal", "auroprint_error", error_msg);
	} catch (...) {
		call_deferred("emit_signal", "auroprint_error", "Unknown error occurred while checking hardware support");
	}
}

void Auroprint::_reset_key_worker() {
	try {
		_platform_reset_key();
		call_deferred("emit_signal", "key_reset_complete");
	} catch (const std::exception &e) {
		String error_msg = String("Failed to reset key: ") + e.what();
		call_deferred("emit_signal", "auroprint_error", error_msg);
	} catch (...) {
		call_deferred("emit_signal", "auroprint_error", "Unknown error occurred while resetting key");
	}
}

void Auroprint::_request_integrity_token_worker(const String &nonce, int64_t cloud_project_number) {
	try {
		String token = _platform_request_integrity_token(nonce, cloud_project_number);
		call_deferred("emit_signal", "integrity_token_received", token);
	} catch (const std::exception &e) {
		String error_msg = String("Failed to request integrity token: ") + e.what();
		call_deferred("emit_signal", "integrity_token_error", error_msg);
	} catch (...) {
		call_deferred("emit_signal", "integrity_token_error", "Unknown error occurred while requesting integrity token");
	}
}

// Platform-specific implementations (stub for desktop/unsupported platforms)
#if !defined(ANDROID_ENABLED) && !defined(IOS_ENABLED)

Ref<AuroprintResult> Auroprint::_platform_generate_auroprint() {
	// Stub implementation for desktop platforms
	Ref<AuroprintResult> result;
	result.instantiate();

	result->set_device_id("desktop-stub-id");
	result->set_payload("{\"did\":\"desktop-stub-id\",\"ts\":0,\"nonce\":\"stub\"}");
	result->set_signature("stub-signature");
	result->set_public_key("stub-public-key");
	result->set_attestation_chain(Array());
	result->set_timestamp(0);
	result->set_nonce("stub-nonce");
	result->set_is_hardware_backed(false);

	UtilityFunctions::push_warning("Auroprint: Desktop platform detected. Returning stub data. Hardware-backed security is only available on Android/iOS.");

	return result;
}

bool Auroprint::_platform_is_hardware_backed_available() {
	return false;
}

void Auroprint::_platform_reset_key() {
	UtilityFunctions::push_warning("Auroprint: reset_key() is not supported on desktop platforms");
}

String Auroprint::_platform_request_integrity_token(const String &nonce, int64_t cloud_project_number) {
	throw std::runtime_error("Integrity tokens are only supported on Android");
	return ""; // Never reached, but satisfies compiler
}

#endif
