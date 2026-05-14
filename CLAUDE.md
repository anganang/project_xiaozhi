# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

AI_xiaozhi — an ESP-IDF project targeting ESP32-S3, implementing an AI assistant device ("小智") with LCD display, LVGL UI, ADC button input, BLE WiFi provisioning, and HTTP/WebSocket communication. Built with ESP-IDF v5.3.1.

## Build Commands

```bash
idf.py build              # Full build
idf.py flash              # Flash to device
idf.py monitor            # Serial monitor (Ctrl+] to exit)
idf.py flash monitor      # Flash and monitor in one step
idf.py menuconfig         # Configure sdkconfig options (WiFi SSID/password, auth mode)
idf.py fullclean          # Clean build artifacts completely
```

Target is set to `esp32s3`. If IDF_PATH is not set, source the ESP-IDF export script first (`export.sh` or `export.ps1`).

## Architecture

- `main/main.c` — Entry point (`app_main`), initializes buttons, LVGL display, and WiFi; registers button event callbacks
- `main/Button/` — ADC button driver wrapping `espressif/button`. Two buttons on ADC1 channel 7: btn2 (0–400mV range) and btn3 (1200–1800mV range)
- `main/wifi_sta/` — WiFi STA with BLE provisioning (`wifi_provisioning/scheme_ble`). First boot triggers BLE provisioning with QR code; subsequent boots connect directly. `wifi_clear_reset()` erases stored credentials and reboots
- `main/xiaozhi_lcd/` — SPI LCD driver (ST7789, 320x240, SPI2). Pin definitions and hardware config live here
- `main/lvgl/` — LVGL UI layer on top of LCD. Provides `xiaozhi_lvgl_set_title()`, `xiaozhi_lvgl_set_emoji()`, `xiaozhi_lvgl_set_dialog()` for updating the display, plus QR code display for provisioning
- `main/http/` — HTTP client for backend communication, uses cJSON for parsing
- `main/xiaozhi_data/` — Global data structures (`xiaozhi_data_t` for WebSocket URL/token/activation state, `EMOJI` array for emotion display)
- `main/decode_image/` + `main/pretty_effect/` — JPEG decoding and display effects for embedded image (`image.jpg`)
- `main/Kconfig.projbuild` — Menuconfig options under "HaHa Configuration": WiFi SSID, password, WPA3 SAE mode, max retry count, auth mode threshold
- `managed_components/` — ESP-IDF component manager dependencies (auto-managed, do not edit)

## Key Dependencies

Declared in `main/idf_component.yml`, resolved via `dependencies.lock`:
- `espressif/button` ^4.1.6 — IoT button component (ADC mode)
- `espressif/qrcode` ^0.2.0 — QR code generation for BLE provisioning
- `espressif/esp_lvgl_port` ^2.7.2 — LVGL port for ESP-IDF
- `espressif/esp_jpeg` ^1.3.1 — JPEG decoder
- `78/xiaozhi-fonts` ^1.6.0 — Custom fonts for the UI

## Dev Environment

A devcontainer config exists (`.devcontainer/`) using the `espressif/idf` Docker image for containerized builds.
