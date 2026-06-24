#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// ---- Callback type ----

/// Output callback.  App 有输出时调用此函数将 JSON 单条消息
/// 推送到 WebSocket。 调用者不拥有 json 指针，回调返回后即失效。
typedef void (*app_output_fn)(void* userdata, const char* json);

// ---- 生命周期 ----

/// Create an app instance.
/// config_json: first client message as JSON string
///   games: `{"action":"new_game","game":"snake","width":20,"height":20}`
///   chat:  `{"text":"/图片"}`
/// Returns opaque handle, or NULL on failure.
void* app_create(const char* config_json);

/// Destroy an app instance.
void app_destroy(void* app);

/// Check whether the session has ended.
/// Returns 0 if active, non-zero if done.
int app_is_done(void* app);

// ---- 异步 API（推荐） ----

/// 投递一条输入消息到 app，立刻返回（非阻塞）。
/// app 处理完毕后通过 app_set_output 指定的回调输出结果。
/// 若 .so 未导出此函数，服务器将回退到同步 app_process。
void app_on_input(void* app, const char* input_json);

/// 注册输出回调。 每个 `cb` 调用携带一条待发送的 JSON 消息。
/// app 生命周期内可多次调用 cb，无需手动释放 json。
/// userdata 会透传给每次 cb 调用。
void app_set_output(void* app, app_output_fn cb, void* userdata);

/// 设置异步 I/O 上下文（可选）。
/// 服务器将 Session 共享的 io_context 传入，app 可在其上做
/// async_read / async_write 而无需自己创建线程。
/// 仅在 app_set_output 之后、app_on_input 之前调用一次。
void app_set_io_context(void* app, void* io_context);

// ---- 同步 API（遗留） ----

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

#ifdef __cplusplus
}
#endif
