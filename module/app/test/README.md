# App 模块单元测试

## 测试文件

| 测试文件 | 测试内容 |
|----------|----------|
| `app_test.cpp` | `AppModule` 默认构造、`AppPtr` 生命周期、`AppModuleCache::load()` mock.so 加载/缓存/驱逐 |
| `mock_app.cpp` | 实现 `app_create`/`app_process`/`app_is_done` 的 mock .so，用于集成测试 |

## 运行

```bash
cd app/build
make tests
./output/test/tests
```

## 测试策略

- **AppModule 测试**：通过头文件构造已足够，无需动态加载
- **AppPtr 测试**：验证 unique_ptr + 自定义 deleter 生命周期
- **AppModuleCache 测试**：构建 mock_app.so(mock_app.cpp) → dlopen → 验证 `load()`/`evict()`/`clear()` 流程
- **不存在模块测试**：验证 `load("nonexistent")` 返回 false 而非崩溃
