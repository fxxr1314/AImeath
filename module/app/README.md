# App —— 应用插件统一加载框架

C++17 库，提供统一 C ABI 和应用插件加载器，编译为共享库 `libapp.so`。

## 特性

- **统一 C ABI**（`app_api.hpp`）—— `app_create` / `app_destroy` / `app_process` / `app_free_string` / `app_is_done`
- **插件加载器**（`app_mod.hpp`）—— `AppModule` / `AppModuleCache` 线程安全的 dlopen 缓存加载器，多路径自动搜索
- **游戏适配器**（`game_app_adapter.cpp`）—— 将游戏传统的 `game_new`/`game_tick`/`game_get_state` C ABI 包装为统一 `app_*` 接口，各 game .so 编译此源文件
- **零配置发现**：加载 `lib<name>.so`，自动搜索 `LD_LIBRARY_PATH`、可执行文件相对路径、开发构建路径

## 目录结构

```
├── include/              头文件
│   ├── app_api.hpp       统一 C ABI 声明（extern "C"）
│   └── app_mod.hpp       AppModule + AppModuleCache + AppPtr
├── src/                  源文件实现
│   ├── app_mod.cpp       AppModuleCache 实现（多路径 dlopen fallback）
│   └── game_app_adapter.cpp  游戏 → 统一 C ABI 适配器（由各 game .so 编译）
├── test/                 单元测试（Google Test）
│   ├── app_test.cpp      测试 AppModuleCache + AppPtr
│   └── mock_app.cpp      测试用 mock .so（导出 app_create / app_process ...）
├── benchmark/            性能测试（Google Benchmark）
│   └── app_bench.cpp     加载/创建/处理/缓存命中 基准测试
├── test_build.sh         快速构建脚本
├── CMakeLists.txt        构建配置
└── README.md             本文档
```

## 依赖

| 依赖 | 用途 |
|------|------|
| C++17 编译器 | 语言标准 |
| CMake ≥ 3.12 | 构建系统 |
| Boost（json, filesystem） | JSON 解析、filesystem 路径操作 |
| Google Test | 测试框架 |
| Google Benchmark | 基准测试框架 |

## 构建

```bash
cd app
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

构建产物：`output/lib/libapp.so`

## 测试

```bash
cd build
make tests
./output/test/tests
```

## 基准测试

```bash
cd build
make bench_app
./output/bench/bench_app --benchmark_min_time=0.1
```

## 设计要点（与 module/game 的职责边界）

| 模块 | 职责 |
|------|------|
| **module/app** | 统一 C ABI 定义、插件加载器、游戏→统一 C ABI 适配器 |
| **module/game** | `Game` 抽象基类、`GAME_API_*` C ABI 宏（游戏专用）、`Direction` 方向工具 |
| **module/core** | 共享基础设施：线程池、日志、网络、事件、定时器、方向工具 |

`game_app_adapter.cpp` 位于 `module/app/src/`，但由各 game .so（而非 libapp.so）编译。它调用 `game_new`/`game_tick`/`game_get_state`（来自 `module/game` 的 C ABI），将其包装为统一的 `app_*` 接口。

## 添加新 app 的步骤

1. 创建 `desktop/app/<name>/backend/CMakeLists.txt`（构建 `.so`，导出 `app_create`/`app_process`/`app_is_done`）
2. 只需链接 `app` 模块以获头文件，无需链接 `libapp.so`
3. 服务器通过 `AppModuleCache::load("<name>")` 自动发现
