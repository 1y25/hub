# 拓展坞屏幕 · STM32F103C8T6 工程

> 基于参考项目 `../CODE/`（hubV2）适配新板子：屏幕接线与参考项目一致（PB10~PB15），W25Q64 片选改为 PA4。

## 1. 引脚连接表

| 信号 | STM32 引脚 | 说明 |
|---|---|---|
| 屏幕 SCK | **PB13** | SPI2 硬件时钟 |
| 屏幕 MOSI/SDA | **PB15** | SPI2 硬件数据 |
| 屏幕 RST | **PB7** | 复位 |
| 屏幕 DC | **PB6** | 数据/命令 |
| 屏幕 CS | **PB5** | 片选 |
| 屏幕背光 BL | **PB14** | 高电平亮（固件驱动） |
| W25Q64 CS | **PB1** | 图片存储 |
| W25Q64 CLK/DI/DO | PA5 / PA7 / PA6 | SPI1 |
| USART1 TX/RX | PA9 / PA10 | 串口烧录图片 + 调试 |
| LED | PA8 | 运行指示（0.4s 灭 / 0.6s 亮，板子上可空） |

## 2. 与参考项目的差异

1. **屏幕驱动**：沿用参考项目的硬件 SPI2（SCK=PB13 / MOSI=PB15）+ DMA 刷屏方案，DC/RST/CS/背光为 PB10/PB11/PB12/PB14。
2. **器件型号**：Keil 工程从 STM32F103C6 改为 **STM32F103C8**（64KB Flash / 20KB RAM），否则固件放不下。
3. **烧录稳定性**：写图期间暂停动画线程（`pic_busy`），避免动画读 W25Q64 与烧录写 W25Q64 的 DMA 通道冲突。
4. **W25Q64 片选**：新板 CS# 接 **PA4**（参考项目是 PB1），已修正；PB1 在新板是 WP#，不再驱动。
5. 清理了 LED 初始化误配 PB15。

## 3. 一、烧录固件

1. 用 **Keil MDK5** 打开 `o/project.uvprojx`。
2. 确认器件为 **STM32F103C8**（Options for Target → Device）。
3. F7 编译，生成 `project.hex`。
4. 用 ST-Link 或串口 ISP（FlyMcu）烧进 STM32F103C8T6。
5. 上电后屏幕先刷**蓝-白-粉三色条**，串口（9600）打印初始化信息 → 驱动正常。

> ⚠️ 晶振：固件按 **8MHz HSE × 9 = 72MHz** 配置。若你板子晶振不是 8MHz，请改 `st_core/system_stm32f10x.c` 中的 `HSE_VALUE`。

## 4. 二、动图制作与烧录（核心流程）

工具在 `TOOL/` 下（需要 `pip install pillow pyserial`）：

```
① 把动图分成帧（PNG/JPG 序列），或直接用 GIF
② python img2c.py ./帧图片目录 --out ./mygif --size 130x130
   （GIF 直接切：python img2c.py anim.gif --out ./mygif --size 130x130）
③ 改 flashpic.py 顶部的 PORT = "COMx"（你的 USB-TTL 串口号）
④ python flashpic.py ./mygif 3 37     ← 烧到第 3 个图库位置，共 37 帧
⑤ 烧完自动重启，屏幕循环播放动画
```

- `img2c.py` 输出 `1.c 2.c ... N.c`（格式与参考项目 Image2LCD 输出完全一致，带 8 字节宽高头、RGB565 小端数据）。
- `--size` 建议 **130x130**（参考项目动画尺寸）；也可不缩放，任意尺寸 ≤ 屏幕可视区域即可，固件会自动居中。
- 若屏幕上颜色红蓝反了：`img2c.py ... --swap-rb` 重新生成。

## 5. 三、串口指令（9600，USB-TTL）

| 指令 | 作用 |
|---|---|
| `COMMAND` / `HELP` | 打印帮助 |
| `CHPIC-x` | 切换到第 x 个图库（1~4） |
| `SetDelay-xx` | 帧间隔（默认 50，越大越慢） |
| `ReadInfo` | 查看 4 个图库的尺寸/帧数/像素量 |
| `RESET` | 重启 |

## 6. 容量参考（W25Q64 = 8MB）

4 个图库：库1 从 1MB、库2 从 3MB、库3 从 5MB、库4 从 7MB，**每库约 2MB 空间**。

- 帧存储布局：**每帧独立 80KB**（20 × 4KB 扇区），帧与帧不重叠、无撕裂
- **160x128 全屏**：每帧 40KB 数据 → 每库最多 **25 帧**
- 130x130：每帧 33.8KB → 每库最多 25 帧（槽位按最大对齐）
- 烧录速度 115200bps，一帧约 3.5 秒，帧多请耐心

## 7. 常见问题

- **屏幕不亮/白屏**：先查背光（PB14 高电平亮，程序已驱动）；再查屏幕排线/5 根信号线有没有接反（SCK=PB13 MOSI=PB15 RST=PB11 DC=PB10 CS=PB12）。
- **图像方向不对**：改 `library/TFT_DMA.c` 里 `TFTD_SoftwareInit()` 的 `0x36`（MADCTL）值。
- **动画偏慢**：硬件 SPI2+DMA 刷一帧约 50ms 以内，`SetDelay` 别低于 20。
- **PA11**（原参考工程呼吸灯 PWM，TIM1_CH4）在新板未接线时无影响；不需要可注释 `Shadow.c` 里的 `Init_PWM()` / `Task_PWM`。
