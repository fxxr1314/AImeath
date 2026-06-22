# src/ —— 源文件实现

## 文件说明

### chat.cpp

`Chat` 类实现 + C API 包装：

- `Chat::process()`：直接调用 `m_fn(input)` 并返回
- `chat_new()`：接收 C 函数指针 `char* (*)(const char*)`，包装为 `std::function`，自动 `free` C 字符串
- `chat_free()`：`delete` Chat 实例
- `chat_process()`：调用 `process()`，将 `std::string` 转为 `malloc` C 字符串返回
