# App 模块基准测试

## 基准测试

| 基准测试 | 测量内容 |
|----------|----------|
| `BM_AppModuleLoad` | `AppModuleCache::load("mock_app")` — dlopen + 符号查找 + 缓存 |
| `BM_AppCreate` | `AppModule::create(config)` — app_create + AppPtr 包装 |
| `BM_AppProcess` | `app_process()` 往返调用 + app_free_string |
| `BM_AppCacheHit` | `cache.load("mock_app")` 缓存命中（无 dlopen） |

## 运行

```bash
cd app/build
make bench_app
./output/bench/bench_app --benchmark_min_time=0.1
```
