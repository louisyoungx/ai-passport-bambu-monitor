# ESP32-2432S028R CYD

本配置面向双 USB（Micro-USB + USB-C）的 2.8 寸 ESP32 CYD：ESP32、4 MB Flash、无 PSRAM、XPT2046 电阻触摸。保持 240×320 竖屏以复用打印机页面，取消 AI Passport 的圆角裁剪。

双 USB 的常见屏幕是 ST7789，但存在其他批次。默认驱动只是初始候选，必须通过实物确认；ILI9341 可在 menuconfig 切换。

## 编译

需要 ESP-IDF 5.5.3（项目组件要求 >=5.5.3,<5.6.0），LVGL 固定为原项目使用的 9.5.0。干净克隆无需创建 secrets.h，缺少该文件时使用占位模板并进入网页配网。Windows PowerShell 先载入已安装的 IDF 环境，再执行：

```powershell
# 本机的安装位置；其他电脑替换为自己的 ESP-IDF 路径。
. "$env:USERPROFILE\.espressif\frameworks\esp-idf-v5.5.3\export.ps1"
.\tools\build-cyd.ps1
.\tools\build-cyd.ps1 -Action menuconfig
.\tools\build-cyd.ps1 -Action flash -Port COM5
.\tools\build-cyd.ps1 -Action monitor -Port COM5
```

COM5 是示例，使用系统识别出的实际串口。连接一根支持数据传输的 USB 线。自动进入下载模式失败时，按住 BOOT，点按 RESET 后松开 BOOT 再烧录。

跨平台等效命令（已激活 ESP-IDF 环境）：

```sh
idf.py -B build-cyd -D SDKCONFIG=sdkconfig.cyd -D SDKCONFIG_DEFAULTS=sdkconfig.defaults.cyd -D IDF_TARGET=esp32 build
```

构建目录为 `build-cyd/`，生成配置为 `sdkconfig.cyd`。它们与原板默认的 `build/`、`sdkconfig` 分开；原 AI Passport 继续按原 README 使用 esp32c3 构建。

`partitions.cyd.csv` 在 4 MB 内分配 3 MB factory 应用和 960 KiB SPIFFS，不提供 OTA。NVS 保留在 0x9000，正常 flash 不擦除凭据。固件名仍为 `build-cyd/passport_printer.bin`，首次烧录应使用 `idf.py flash` 同时写入 bootloader 和分区表，不能只写应用 bin。

## 操作

| 输入 | 导航动作 | 作用 |
| --- | --- | --- |
| 向左滑动至少 40 px | 下一页 | 打印机详情页直接切到下一页，控制页也能直接离开 |
| 向右滑动至少 40 px | 上一页 | 打印机详情页直接切到上一页 |
| 向上滑动至少 40 px | DOWN | 下一页 / 下一项 |
| 向下滑动至少 40 px | UP | 上一页 / 上一项 |
| 轻点屏幕 | OK 单击 | 确认、刷新或执行当前选中操作 |
| 原地按住至少 500 ms | OK 长按 | 返回概览 / 菜单 |
| BOOT 单击 | DOWN | 下一页 / 下一项 |
| BOOT 双击（300 ms 内） | OK 单击 | 确认 |
| BOOT 按住至少 500 ms | OK 长按 | 返回 |

左右滑动使用独立切页事件，上下滑动沿用原按键导航，轻点确认当前选中操作。抬手事件不会覆盖等待 LVGL 处理的切页或确认事件。首次未配置时自动显示配网热点；配网流程沿用原 README。

CYD 永不自动息屏，背光与 LVGL 持续运行；AI Passport 保留原来的 30 秒空闲息屏策略。CYD 配置关闭自动 light sleep。菜单/多打印机概览中左右滑动可选择上一项/下一项，上下滑动沿用原按键导航语义；在控制页用上下滑动选择按钮、轻点执行，左右滑动直接切页。开机动画期间不接受导航。

## 接线与外设

接线定义经 `bsp_pins.h` 路由至 `bsp_pins_cyd.h`，驱动、颜色、镜像与触摸校准在 `menuconfig → Board support` 配置。

| 外设 | 配置 |
| --- | --- |
| LCD / SPI2 | MOSI 13、SCLK 14、CS 15、DC 2、RST -1（软复位）、40 MHz |
| 背光 | GPIO21，5 kHz PWM，高电平点亮 |
| XPT2046 / SPI3 | MOSI 32、MISO 39、SCLK 25、CS 33、IRQ 36，1 MHz |
| BOOT | GPIO0，低电平有效 |
| 音频 | 内置 DAC GPIO26 → 板载功放 → 外接扬声器；16 bit 单声道 PCM 转 8 bit DAC |
| 电量 | 没有 CW2017，不访问电量计，状态栏显示 USB |
| I²C 扩展 | SDA 27 / SCL 22；启动不扫描，不访问 ES8311 |

没有麦克风录音支持。SD 卡、RGB 灯、光敏电阻不属于本监控应用当前功能；本次不初始化它们。尤其不要把 SD 卡按其原生引脚再次初始化到 SPI3：触摸已经使用 SPI3 的另一组引脚，需要另行规划总线共享。

## 屏幕和触摸调试

1. 黑屏先确认背光与日志。如果只有背光、没有画面，在 Board support 切换 ST7789 / ILI9341；必要时调整 LCD SPI mode。
2. 红蓝交换时切换 BGR；画面负片时切换 Invert display colors；方向不对时调整 Mirror X/Y。不要把 AI Passport 的 ST7789P3 厂商 gamma 序列复制给 CYD。
   当前实物已反馈整幅画面呈负片，因此 CYD 默认关闭反色（`CONFIG_BSP_CYD_LCD_INVERT`），RGB/BGR 顺序保持原配置。
3. 触摸默认按竖屏进行轴交换和 X 反向。开启 Log raw touch coordinates，观察四角原始值，调整 X/Y 最小最大值及 Swap / Invert 设置。边界必须满足 max > min；设置无效会拒绝启动输入。
4. 本次默认参数未经该块实物校准。滑动方向相反时，先调整触摸 Y 反向，再测试点击、滑动与长按。

当前 CYD 默认配置还开启了 `Rotate display and gestures 180 degrees`（`CONFIG_BSP_CYD_ROTATE_180=y`），按用户要求在镜像修正后的画面上再旋转半圈，触摸坐标同步旋转。需要恢复原方向时关闭这一项；无需改动面板自身的 Mirror X/Y 或重新标定原始触摸范围。此版本已重新编译、写入 COM3 并通过哈希校验；启动日志确认最终面板参数为 `mirror_x=0 mirror_y=1`、输入旋转为 `rotation=180`，配网 HTTP 服务正常启动。实际旋转方向及触摸手感仍待用户确认。

## 实物验收

2026-11-06 编译验证：ESP-IDF 5.5.3 + LVGL 9.5.0 下，CYD ST7789 与 ILI9341 两种配置均完成完整构建；应用约 1.90 MiB，3 MiB 应用分区剩余约 37%。PowerShell 构建脚本语法检查及实际一键构建均通过。生成配置中的芯片、Flash、驱动、56 KiB 内存池与 PM 设置已核对，分区无重叠且末尾恰好为 4 MiB。

原 ESP32-C3 / AI Passport 配置也通过独立目录的完整回归构建，保留 8 MB Flash、原面板初始化、ADC 三键、ES8311、CW2017 与 USB-Serial-JTAG 控制台。

2026-11-07 实物记录：COM3 上确认为 ESP32-D0WD-V3 rev3.1、4 MB Flash。首次 ST7789 固件烧录及哈希校验通过；22 秒启动日志中，显示/LVGL/输入初始化成功，热点 Bambu-Setup 和 HTTP 配网服务正常启动，未见崩溃。用户反馈镜像后，开启面板水平镜像修正并重刷；用户确认镜像已修复，随后要求在此基础上旋转 180°，因此加入独立的显示/触摸同步旋转选项。尚未完成触摸、音频、联网监控等其余实物验收。

仓库的 `tools/check_font.py` 当前返回 FAIL：它要求 20 号标题字库覆盖所有正文字符。已在同一环境对比原始 HEAD，诊断完全一致，14 号字库及两套字体的 ASCII 覆盖通过。本次未新增中文缺字。

2026-11-07 配置迁移与反色修正：从本地 AI Passport 备份导入了 1 组 Wi-Fi、拓竹账号 ID/Token 和 1 台打印机配置。迁移仅更新应用命名空间，CYD 原有 27 项系统/校准配置经生成前后比对保留，NVS 写入后回读完全一致。用户确认画面整体像负片后，关闭屏幕反色并重新编译烧录；烧录前后 NVS 逐字节一致。新启动日志确认 `invert=0`、`rotation=180`，Wi-Fi 和拓竹云均连接成功，未见崩溃。实际画面颜色仍待用户确认。

2026-11-07 触屏/常亮更新：加入左右独立切页事件，修正抬手事件覆盖待处理点击/切页事件的问题；关闭 CYD 自动息屏路径。编译与烧录校验通过，烧录前后 NVS 逐字节一致。启动日志确认常亮策略生效，连续观察 80 秒未触发自动息屏，Wi-Fi/拓竹云正常连接且无崩溃。随后用户在实物上确认左右滑动切页正常。

- [x] 串口识别为 ESP32，Flash 探测为 4 MB；启动没有 GPIO/驱动错误。
- [ ] 开机动画与所有页面完整显示；红绿蓝、方向正确；四角没有黑色圆角遮罩。
- [ ] 滑动双向切页、轻点确认、长按返回、BOOT 单击/双击/长按均可用。
- [x] CYD 常亮策略生效，连续 80 秒启动日志未触发自动息屏。
- [x] 用户在实物上确认左右滑动切页正常。
- [ ] 控制页上下选择经实物手势确认。
- [ ] 热点配网、保存凭据、重启重连、打印机 MQTT 实时数据正常。
- [ ] 连接扬声器后完成/失败/HMS 提示音可用，播放时触摸仍正常（GPIO25 不被 DAC 占用）。
- [ ] 连续联网和切页无内存耗尽、看门狗或重启。

代码编译结果与硬件验收需分别记录；未连接实物时，不能视为已完成上述验收。

## 资料

- [CYD 引脚](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/PINS.md)
- [ESPHome CYD 屏幕变体](https://github.com/esphome/esphome/blob/dev/esphome/components/mipi_spi/models/cyd.py)
- [Espressif ILI9341 驱动](https://components.espressif.com/components/espressif/esp_lcd_ili9341/versions/2.1.0/readme)
- [ESP-IDF 5.5.3 ESP32 DAC](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32/api-reference/peripherals/dac.html)
