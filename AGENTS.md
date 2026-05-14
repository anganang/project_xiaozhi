# AGENTS.md

本文件给 Codex/Agent 在本仓库工作时使用。通用交互和改代码原则以用户提供的全局规则为准；这里仅补充 `AI_xiaozhi` 项目特有约束。

## 项目概况

这是一个 ESP-IDF C 工程，项目名 `AI_xiaozhi`，目标芯片为 ESP32-S3。当前功能覆盖 LCD/ST7789 显示、LVGL UI、ADC 按键、BLE WiFi 配网、HTTP 通信、JPEG 显示效果，以及 ES8311/I2S 音频输入输出。

主要业务代码在 `main/`：

- `main/main.c`：`app_main` 入口和按键事件回调注册。
- `main/Button/`：ADC 按键封装，依赖 `espressif/button`。
- `main/wifi_sta/`：WiFi STA 与 BLE provisioning，`wifi_clear_reset()` 会清除 WiFi 配置并重启。
- `main/xiaozhi_lcd/`：ST7789 SPI LCD 驱动和硬件引脚配置。
- `main/lvgl/`：LVGL UI 封装，包括标题、表情、对话文本和配网二维码显示。
- `main/http/`：HTTP 客户端与 cJSON 解析。
- `main/xiaozhi_data/`：全局数据结构和表情数据。
- `main/decode_image/`、`main/pretty_effect/`：嵌入式 JPEG 解码和显示效果。
- `main/Aud/`：ES8311 codec、I2C/I2S 音频初始化与读写。

## 不要编辑的内容

`build/` 和 `build_codex_check/` 是构建产物，默认不要改。`managed_components/` 由 ESP-IDF Component Manager 管理，默认不要直接修改；需要调整依赖时改 `main/idf_component.yml`，再让 ESP-IDF 重新解析。不要批量删除文件或目录；确实需要删除时只能一次删除一个明确路径的文件。

## 构建与验证

优先使用 ESP-IDF 命令验证改动：

```powershell
idf.py build
```

常用命令：

```powershell
idf.py menuconfig
idf.py flash
idf.py monitor
idf.py flash monitor
```

如果当前 shell 没有 ESP-IDF 环境，先加载 ESP-IDF export 脚本，例如 PowerShell 下通常是：

```powershell
.\export.ps1
```

具体路径取决于本机 ESP-IDF 安装位置，不要在仓库中硬编码绝对路径。无法运行 `idf.py build` 时，在最终回复中明确说明原因和未验证风险。

## 配置与依赖

项目根 `CMakeLists.txt` 使用 ESP-IDF 标准 `project(AI_xiaozhi)`。`main/CMakeLists.txt` 明确列出源文件和 include 目录，并通过 `EMBED_FILES image.jpg` 嵌入 `main/image.jpg`；新增 `.c` 文件时必须同步加入 `main/CMakeLists.txt`。

`main/idf_component.yml` 是依赖入口，当前包含：

- `espressif/button`
- `espressif/qrcode`
- `espressif/esp_lvgl_port`
- `espressif/esp_jpeg`
- `78/xiaozhi-fonts`
- `espressif/esp_codec_dev`
- `espressif/esp-sr`

`main/Kconfig.projbuild` 的配置菜单名为 `HaHa Configuration`，包含 WiFi SSID、密码、重试次数和认证模式阈值等选项。涉及 WiFi 默认值、配网行为或认证策略时，先检查这里和 `sdkconfig` 的关系。

## 代码风格

现有代码是 C，函数和文件名主要使用 `xiaozhi_*` 前缀。新增接口优先沿用同一命名风格，并保持 `.c`/`.h` 成对放在对应模块目录。注释可以使用中文；不要为了统一风格改写已有注释或格式。

硬件引脚和协议参数分散在各模块实现中，例如 LCD、ADC button、I2C/I2S audio。改这些常量前要确认对应硬件连接；没有明确需求时不要顺手调整引脚、采样率、屏幕尺寸、WiFi provisioning 安全参数或音量增益。

## 已知注意点

`README.md` 仍是 ESP-IDF 示例模板，不能作为项目真实架构依据；优先参考本文件、`CLAUDE.md`、`main/` 源码和 CMake 配置。

仓库当前可能不是 Git 工作树；不要依赖 `git status` 判断用户改动。做修改前后用文件读取和构建结果确认范围。
