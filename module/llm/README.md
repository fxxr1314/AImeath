# LLM —— 异步流式 LLM API 客户端模块

C++17 库，提供基于 Boost.Asio 的异步 HTTPS 流式 LLM 客户端，兼容 OpenAI Chat Completions 接口（SSE 流式响应）。编译为共享库 `libllm.so`。

## 特性

- **异步流式请求** — 基于 Boost.Beast SSL 流的全异步实现，不阻塞调用线程
- **SSE 解析** — 自动解析 Server-Sent Events，提取 delta 内容与推理内容
- **超时控制** — 可配置请求超时时间
- **取消支持** — 安全取消进行中的请求
- **兼容性** — 适配 DeepSeek / OpenAI 等 Chat Completions API

## 目录结构

```
├── include/              头文件
│   ├── llm_client.hpp    LlmClient 声明
│   └── README.md         头文件说明
├── src/                  源文件实现
│   ├── llm_client.cpp    LlmClient 实现
│   └── README.md         源文件说明
├── test/                 单元测试（Google Test）
│   ├── llm_test.cpp      测试用例
│   └── README.md         测试说明
├── benchmark/            性能测试（Google Benchmark）
│   ├── llm_bench.cpp     基准测试
│   └── README.md         基准测试说明
├── test_build.sh         快速构建脚本
├── CMakeLists.txt        构建配置
└── README.md             本文档
```

## 依赖

| 依赖 | 用途 |
|------|------|
| C++17 编译器 | 语言标准 |
| CMake ≥ 3.12 | 构建系统 |
| Boost（json, asio, beast） | 异步网络、JSON 解析 |
| OpenSSL | HTTPS/SSL 支持 |
| Google Test | 测试框架 |
| Google Benchmark | 基准测试框架 |

## 构建

```bash
cd llm
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

构建产物：`output/lib/libllm.so`

## 测试

```bash
cd build
./output/test/tests
```

## 基准测试

```bash
cd build
make bench
```

## 使用示例

```cpp
#include "llm_client.hpp"
#include <boost/asio.hpp>

boost::asio::io_context io;
auto client = std::make_shared<LlmClient>(io);

client->start("api.deepseek.com", "443",
    R"({"model":"deepseek-chat","messages":[{"role":"user","content":"hello"}],"stream":true})",
    "Bearer sk-xxx",
    [](LlmEvent ev) {
        if (ev.type == "delta")
            std::cout << ev.text;
        else if (ev.type == "reasoning")
            std::cout << "[reasoning] " << ev.text;
    },
    [](std::string response, std::string reasoning) {
        std::cout << "\n[Done] response=" << response;
    });

io.run();
```

## 设计要点

- `LlmClient` 通过 `std::enable_shared_from_this` 管理异步回调生命周期，确保回调安全访问
- 内部 `Impl` 模式隐藏实现细节，头文件不暴露 Boost 类型
- SSE 解析兼容 DeepSeek 的 `reasoning_content` 字段和 OpenAI 标准的 `content` 字段
- 超时通过 `asio::steady_timer` 实现，cancel 时自动取消所有异步操作
- 析构时自动 cancel 进行中的请求（通过 `unique_ptr<Impl>` 的 RAII 语义）
