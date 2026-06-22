# include/ —— 头文件

聊天模块的公共接口定义，编译为共享库后通过 `dlopen` 动态加载。

## 文件说明

### chat.hpp

`Chat` 类定义 + `extern "C"` C API：

- `MeowFn` 类型别名：`std::function<std::string(const std::string&)>`
- `Chat(MeowFn fn)`：构造函数，注入处理回调
- `process(input)`：对输入文本调用回调，返回处理结果
- C API：`chat_new` / `chat_free` / `chat_process`
