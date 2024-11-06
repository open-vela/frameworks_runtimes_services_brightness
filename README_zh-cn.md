# 介绍

[[English](./README.md)|简体中文]

亮度服务提供基于光传感器输入的自动屏幕亮度控制。它还可以响应用户输入相应调整控制曲线。它受到Android亮度服务的巨大启发。

该服务依赖于包括libuv、uorb和视频驱动在内的几个其他模块。如果启用了`KVDB`，用户设置将会持久化。

# 项目结构

```tree
├── abc.c
├── display.c
├── lightsensor.c
├── persist.c
├── spline.c
├── aidl
├── README.md
└── README_zh-cn.md
```

* `abc`: 自动亮度控制器的缩写，是控制器的主要实现
* `display`: 设置/获取亮度并处理平滑过渡。
* `lightsensor`: 管理传感器输入和数据过滤。
* `persist`: 使用KVDB存储/恢复用户设置。
* `spline`: 计算用户添加的控制点。
* `aidl`: 添加AIDL层以将亮度服务暴露给其他任务。

# 使用方法

## 启动服务

亮度服务应跟随systemd启动。

## 与亮度服务交互

建议使用AIDL接口控制亮度模式和设置亮度级别。`include/BrightnessServiceC.h`中提供了C语言接口。

要设置亮度模式，只需调用`BrightnessService_setBrightnessMode`:
  * `BRIGHTNESS_MODE_AUTO`是让控制器自动读取传感器并决定当前亮度级别。
  * `BRIGHTNESS_MODE_MANUAL`允许将亮度设置为任何值。

两个特殊的亮度级别`BRIGHTNESS_LEVEL_OFF`和`BRIGHTNESS_LEVEL_FULL`，可以用于关闭显示或设置为全亮。

如果启用了KVDB，用户设置（包括模式和级别）将自动保存，并在下次启动时恢复。
