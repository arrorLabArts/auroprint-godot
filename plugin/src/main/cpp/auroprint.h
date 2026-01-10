#ifndef AUROPRINT_H
#define AUROPRINT_H

#include "auroprint_result.h"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

using namespace godot;

class Auroprint : public RefCounted {
	GDCLASS(Auroprint, RefCounted)

private:
	static Auroprint *singleton;

protected:
	static void _bind_methods();

public:
	Auroprint();
	~Auroprint();

	static Auroprint *get_singleton();

	// Public API methods (async via signals)
	void generate_auroprint();
	void is_hardware_backed_available();
	void reset_key();
	void request_integrity_token(const String &nonce, int64_t cloud_project_number);

	// Worker methods (called on background thread)
	void _generate_auroprint_worker();
	void _is_hardware_backed_available_worker();
	void _reset_key_worker();
	void _request_integrity_token_worker(const String &nonce, int64_t cloud_project_number);

	// Platform-specific implementations (declared here, defined in platform files)
#ifdef ANDROID_ENABLED
	Ref<AuroprintResult> _platform_generate_auroprint();
	bool _platform_is_hardware_backed_available();
	void _platform_reset_key();
	String _platform_request_integrity_token(const String &nonce, int64_t cloud_project_number);
#elif defined(IOS_ENABLED)
	Ref<AuroprintResult> _platform_generate_auroprint();
	bool _platform_is_hardware_backed_available();
	void _platform_reset_key();
	String _platform_request_integrity_token(const String &nonce, int64_t cloud_project_number);
#else
	// Desktop/other platforms - stub implementations
	Ref<AuroprintResult> _platform_generate_auroprint();
	bool _platform_is_hardware_backed_available();
	void _platform_reset_key();
	String _platform_request_integrity_token(const String &nonce, int64_t cloud_project_number);
#endif
};

#endif // AUROPRINT_H
