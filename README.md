# Introduction

[English|[简体中文](./README_zh-cn.md)]

THe brightness service provides automatic screen brightness control based on
light sensor input. It can also respond to user input to adjust the control
curve correspondingly. It hugely inspired by Android brightness service.

This service relies on serval other modules including libuv, uorb, and video
driver. If `KVDB` is enabled, user settings will be persist.

# Project Structure

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

* `abc`: Short for auto-brightness-controller, is the main controller.
* `display`: It set/get brightness and also handles smooth transition.
* `lightsensor`: Manages sensor input and data filter.
* `persist`: Use KVDB to store/restore user settings.
* `spline`: Calculates the user added control point.
* `aidl`: Adds AIDL layer to expose brightness services to other tasks.

# Usage

## Start the service

The brightness service should start with systemd.

## Interact with brightness service

It's recommended to use the AIDL interface to control brightness mode and
set brightness level. A C wrapper is provided in `include/BrightnessServiceC.h`.

To set brightness mode, simply call `BrightnessService_setBrightnessMode`.

* `BRIGHTNESS_MODE_AUTO` Auto mode is to let the controller automatically read sensor and decides current
brightness level.
* `BRIGHTNESS_MODE_MANUAL` Manual mode allows to set the brightness to any value.

Two special brightness level `BRIGHTNESS_LEVEL_OFF` and `BRIGHTNESS_LEVEL_FULL`
are provided to set turn off display or to full brightness.

If `KVDB` is enabled, the user settings including mode and level are
automatically saved and will be restored upon next power up.
