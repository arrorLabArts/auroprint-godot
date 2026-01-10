#ifndef AUROPRINT_RESULT_H
#define AUROPRINT_RESULT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

using namespace godot;

class AuroprintResult : public RefCounted {
	GDCLASS(AuroprintResult, RefCounted)

private:
	String device_id;
	String payload;
	String signature;
	String public_key;
	Array attestation_chain;  // Array of Strings
	int64_t timestamp;
	String nonce;
	bool is_hardware_backed;

protected:
	static void _bind_methods();

public:
	AuroprintResult();
	~AuroprintResult();

	// Getters
	String get_device_id() const;
	String get_payload() const;
	String get_signature() const;
	String get_public_key() const;
	Array get_attestation_chain() const;
	int64_t get_timestamp() const;
	String get_nonce() const;
	bool get_is_hardware_backed() const;

	// Setters (for platform layer)
	void set_device_id(const String &p_device_id);
	void set_payload(const String &p_payload);
	void set_signature(const String &p_signature);
	void set_public_key(const String &p_public_key);
	void set_attestation_chain(const Array &p_chain);
	void set_timestamp(int64_t p_timestamp);
	void set_nonce(const String &p_nonce);
	void set_is_hardware_backed(bool p_is_hw_backed);

	// Utility method
	Dictionary to_dictionary() const;
};

#endif // AUROPRINT_RESULT_H
