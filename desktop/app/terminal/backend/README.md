# 命令行后端

C++ Shell 命令透传后端，通过 `app_api.hpp` C ABI 导出。

## 接口

- `app_create` — 创建实例
- `app_destroy` — 销毁实例
- `app_process` — 处理 `{ action: "exec", cmd: "..." }`，返回执行结果
- `app_free_string` — 释放返回字符串
- `app_is_done` — 检查是否结束（永不结束）

## 构建

```bash
cd desktop/app/terminal/backend
mkdir build && cd build
cmake ..
make
```

输出 `libterminal.so` 到 `build/output/lib/`。
