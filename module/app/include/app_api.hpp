#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Create an app instance.
/// config_json: first client message as JSON string
///   games: `{"action":"new_game","game":"snake","width":20,"height":20}`
///   chat:  `{"text":"/图片"}`
/// Returns opaque handle, or NULL on failure.
void* app_create(const char* config_json);

/// Destroy an app instance.
void app_destroy(void* app);

/// Process an input message and produce output message(s).
/// input_json: single client message as JSON string
/// Returns: JSON array string of response objects, e.g.:
///   `[{"type":"snake","grid":"...","score":0,"over":false}]`
///   `[{"type":"stream_start"},{"type":"delta","text":"..."},{"type":"stream_end"}]`
///   `[]` for no output
/// Caller must free the result via app_free_string.
char* app_process(void* app, const char* input_json);

/// Free a string returned by app_process.
void app_free_string(char* str);

/// Check whether the session has ended.
/// Returns 0 if active, non-zero if done.
int app_is_done(void* app);

#ifdef __cplusplus
}
#endif
