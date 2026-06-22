# test/ —— 单元测试（Google Test）

共 6 个测试用例，覆盖 Chat 类的所有核心逻辑。

## 文件说明

### chat_test.cpp

**ChatTest（6 个）**
- `ProcessAppendsMeow`：空字符串、英文、中文输入均正确追加
- `ProcessMultipleCalls`：多次调用返回正确结果
- `ProcessWithChineseInput`：中文字符与"喵"混合
- `CapiNewFree`：C API 创建/销毁不崩溃
- `CapiProcess`：C API 处理文本返回正确结果
- `CapiMultipleInstances`：多个独立实例并行处理

## 运行

```bash
cd build
./output/test/tests
```
