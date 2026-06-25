# LLM 模块头文件说明

## llm_client.hpp

异步流式 LLM API 客户端，兼容 OpenAI Chat Completions 接口（SSE 流式响应）。

- `LlmClient` — 基于 Boost.Asio io_context 的异步 HTTPS 流式客户端
- `LlmEvent` — 流式事件结构体，含 type（delta/reasoning/stream_end/error）、text、msg
- `LlmEventFn` — 事件回调类型
- `LlmDoneFn` — 完成回调类型（返回完整响应文本与推理文本）
