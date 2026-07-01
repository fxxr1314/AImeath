# agent — AI Agent 模块

## 概述

agent 模块实现 AI Agent 功能：通过 LLM（DeepSeek）推理 + 工具调用机制，
让 AI 能够打开、操控和关闭桌面应用程序。

## 架构

```
用户 → Agent 聊天 UI → WebSocket → libagent.so → DeepSeek API (LLM)
                              ↓
                      function_call: open_app / control_app / close_app
                              ↓
                      前端 postMessage → HomePage → 应用窗口操作
```

## C ABI

标准 app C ABI（参见 `app_api.hpp`）：
- `app_create` / `app_destroy` — 生命周期
- `app_on_input` / `app_set_output` — 异步 API
- `app_process` / `app_free_string` — 同步回退
- `app_is_done` — 状态查询

## C++ API

```cpp
#include "agent_api.hpp"
// IAgent 接口供其他模块调用
class IAgent {
    virtual bool openApp(const std::string& name, paramsJson) = 0;
    virtual bool controlApp(const std::string& name, commandJson) = 0;
    virtual bool closeApp(const std::string& name) = 0;
    virtual void stop() = 0;
};
```

## 工具定义

Agent 通过 DeepSeek function_call 使用以下工具：

| 工具        | 参数                         | 描述       |
|------------|------------------------------|-----------|
| open_app   | app(string), width(int), height(int) | 打开应用   |
| control_app | app(string), value(int)     | 操控应用    |
| close_app  | app(string)                 | 关闭应用    |

## 通信协议

Agent 通过 WebSocket 输出 agent_action 消息：

```json
{"type":"agent","action":"open_app","app":"snake","params":{"width":20,"height":20}}
{"type":"agent","action":"control_app","app":"snake","command":{"value":3}}
{"type":"agent","action":"close_app","app":"snake"}
```

前端 agent iframe 通过 `postMessage` 将这些消息转发给 HomePage 处理。

## 依赖

- `libcore.so` — iface_mod.hpp, message_queue.hpp
- `libllm.so` — LlmClient (DeepSeek API)
- Boost.Json — JSON 序列化
