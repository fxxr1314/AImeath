# 命令行应用

基于 Web 的终端模拟器，在 `desktop/public/home` 路径下提供类 Unix 命令行环境。

## 功能

- 虚拟文件系统，映射 `/home`（即 `desktop/public/home/`）目录结构
- 支持命令：`ls`、`cd`、`pwd`、`cat`、`echo`、`clear`、`help`、`whoami`、`date`、`uname`、`exit`
- 命令历史（↑/↓ 方向键）
- 绿色主题终端 UI

## 目录结构

```
frontend/
├── config.js      # 应用配置与图标
├── index.vue      # 终端组件
└── README.md      # 本文档
```
