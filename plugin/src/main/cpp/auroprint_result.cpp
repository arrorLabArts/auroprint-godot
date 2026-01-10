#include "auroprint_result.h"

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

AuroprintResult::AuroprintResult() {
	device_id = "";
	payload = "";
	signature = "";
	public_key = "";
	attestation_chain = Array();
	timestamp = 0;
	nonce = "";
	is_hardware_backed = false;
}

AuroprintResult::~AuroprintResult() {
}

void AuroprintResult::_bind_methods() {
	// Bind getters
	ClassDB::bind_method(D_METHOD("get_device_id"), &AuroprintResult::get_device_id);
	ClassDB::bind_method(D_METHOD("get_payload"), &AuroprintResult::get_payload);
	ClassDB::bind_method(D_METHOD("get_signature"), &AuroprintResult::get_signature);
	ClassDB::bind_method(D_METHOD("get_public_key"), &AuroprintResult::get_public_key);
	ClassDB::bind_method(D_METHOD("get_attestation_chain"), &AuroprintResult::get_attestation_chain);
	ClassDB::bind_method(D_METHOD("get_timestamp"), &AuroprintResult::get_timestamp);
	ClassDB::bind_method(D_METHOD("get_nonce"), &AuroprintResult::get_nonce);
	ClassDB::bind_method(D_METHOD("get_is_hardware_backed"), &AuroprintResult::get_is_hardware_backed);

	// Bind setters
	ClassDB::bind_method(D_METHOD("set_device_id", "device_id"), &AuroprintResult::set_device_id);
	ClassDB::bind_method(D_METHOD("set_payload", "payload"), &AuroprintResult::set_payload);
	ClassDB::bind_method(D_METHOD("set_signature", "signature"), &AuroprintResult::set_signature);
	ClassDB::bind_method(D_METHOD("set_public_key", "public_key"), &AuroprintResult::set_public_key);
	ClassDB::bind_method(D_METHOD("set_attestation_chain", "attestation_chain"), &AuroprintResult::set_attestation_chain);
	ClassDB::bind_method(D_METHOD("set_timestamp", "timestamp"), &AuroprintResult::set_timestamp);
	ClassDB::bind_method(D_METHOD("set_nonce", "nonce"), &AuroprintResult::set_nonce);
	ClassDB::bind_method(D_METHOD("set_is_hardware_backed", "is_hardware_backed"), &AuroprintResult::set_is_hardware_backed);

	// Bind utility method
	ClassDB::bind_method(D_METHOD("to_dictionary"), &AuroprintResult::to_dictionary);

	// Add properties for editor convenience
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "device_id"), "set_device_id", "get_device_id");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "payload"), "set_payload", "get_payload");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "signature"), "set_signature", "get_signature");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "public_key"), "set_public_key", "get_public_key");
	ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "attestation_chain"), "set_attestation_chain", "get_attestation_chain");
	ADD_PROPERTY(PropertyInfo(Variant::INT, "timestamp"), "set_timestamp", "get_timestamp");
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "nonce"), "set_nonce", "get_nonce");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "is_hardware_backed"), "set_is_hardware_backed", "get_is_hardware_backed");
}

// Getters
String AuroprintResult::get_device_id() const {
	return device_id;
}

String AuroprintResult::get_payload() const {
	return payload;
}

String AuroprintResult::get_signature() const {
	return signature;
}

String AuroprintResult::get_public_key() const {
	return public_key;
}

Array AuroprintResult::get_attestation_chain() const {
	return attestation_chain;
}

int64_t AuroprintResult::get_timestamp() const {
	return timestamp;
}

String AuroprintResult::get_nonce() const {
	return nonce;
}

bool AuroprintResult::get_is_hardware_backed() const {
	return is_hardware_backed;
}

// Setters
void AuroprintResult::set_device_id(const String &p_device_id) {
	device_id = p_device_id;
}

void AuroprintResult::set_payload(const String &p_payload) {
	payload = p_payload;
}

void AuroprintResult::set_signature(const String &p_signature) {
	signature = p_signature;
}

void AuroprintResult::set_public_key(const String &p_public_key) {
	public_key = p_public_key;
}

void AuroprintResult::set_attestation_chain(const Array &p_chain) {
	attestation_chain = p_chain;
}

void AuroprintResult::set_timestamp(int64_t p_timestamp) {
	timestamp = p_timestamp;
}

void AuroprintResult::set_nonce(const String &p_nonce) {
	nonce = p_nonce;
}

void AuroprintResult::set_is_hardware_backed(bool p_is_hw_backed) {
	is_hardware_backed = p_is_hw_backed;
}

// Utility method
Dictionary AuroprintResult::to_dictionary() const {
	Dictionary dict;
	dict["device_id"] = device_id;
	dict["payload"] = payload;
	dict["signature"] = signature;
	dict["public_key"] = public_key;
	dict["attestation_chain"] = attestation_chain;
	dict["timestamp"] = timestamp;
	dict["nonce"] = nonce;
	dict["is_hardware_backed"] = is_hardware_backed;
	return dict;
}
