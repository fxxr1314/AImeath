# 源文件说明

## llm_client.cpp

`LlmClient` 实现文件。

- 基于 Boost.Beast SSL 流实现异步 HTTPS 请求
- 解析 Server-Sent Events (SSE) 流式响应
- 支持 cancel / timeout
- 兼容 DeepSeek / OpenAI Chat Completions API 格式
