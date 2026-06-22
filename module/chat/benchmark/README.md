# benchmark/ —— 性能基准测试（Google Benchmark）

## 文件说明

### chat_bench.cpp

- `BM_ChatProcessShort` — 短文本（"hello"）处理性能
- `BM_ChatProcessLong` — 长文本（1000 字符）处理性能
- `BM_ChatProcessChinese` — 中文文本处理性能

## 运行

```bash
cd build
./output/bench/bench_chat
```
