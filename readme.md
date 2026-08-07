# 拓展坞 + 屏幕 · STM32F103C8T6

USB 拓展坞板载 **1.8 寸 ST7735 屏幕**，通过串口把**动图**烧进 W25Q64 外挂 Flash，上电自动循环播放。

## 快速开始（clone 下来就能用）

```
git clone https://github.com/TakoyakiFuwa/hub.git
```

### ① 硬件接线

| 屏幕信号 | STM32 引脚 |
|---|---|
| SCK | PB13（SPI2 硬件） |
| MOSI | PB15（SPI2 硬件） |
| RST | PB7 |
| DC | PB6 |
| CS | PB5 |
| 背光 BL | PB14（高电平亮） |

| W25Q64 | STM32 引脚 |
|---|---|
| CS | PB1 |
| CLK | PA5 |
| DI(MOSI) | PA7 |
| DO(MISO) | PA6 |

串口：PA9(TX) / PA10(RX)，USB-TTL 接 COM 口。

### ② 编译固件（只需一次）

1. 用 **Keil MDK5** 打开 `CODE_MYHUB/o/project.uvprojx`
2. 器件确认 **STM32F103C8**（Options for Target → Device）
3. **F7 编译** → **LOAD 烧录**（ST-Link）
4. 上电屏幕先刷三色条 → 播放 W25Q64 里已有的动画

### ③ 动图制作 + 烧录（每次换动画）

需要 Python 3 + `pip install pillow pyserial`：

```
python CODE_MYHUB/TOOL/img2c.py 动图.gif --out .\mygif --size 160x128 --frames 25 --autocenter
```

改 `CODE_MYHUB/TOOL/flashpic.py` 顶部 `PORT = "COMx"`（你的串口号），然后：

```
python CODE_MYHUB/TOOL/flashpic.py .\mygif 2 25
```

烧完自动重启播放。**每库最多 25 帧（160x128 全屏），共 4 个图库**。

### ④ 串口指令（9600）

| 指令 | 作用 |
|---|---|
| `CHPIC-x` | 切换图库 1~4 |
| `SetDelay-xx` | 帧间隔（默认 50） |
| `ReadInfo` | 查看各图库信息 |
| `COMMAND` | 帮助 |

## 工具一览（`CODE_MYHUB/TOOL/`）

- `img2c.py` — 动图/图片 → 屏幕帧 `.c` 文件（支持 GIF 拆帧、抽帧、自动居中）
- `flashpic.py` — 串口把帧烧进 W25Q64
- `upres_c.py` — 旧 `.c` 帧直接转 160x128 全屏

## 目录结构

```
CODE_MYHUB/      ← 本项目的 Keil 工程 + 工具 + 说明
CODE/            ← 参考项目（hubV2 原作者工程，未改动）
TOOL/            ← 原版上位机工具
EDA/             ← 立创 EDA 工程
```

## 详细文档

- 本项目完整说明：**`CODE_MYHUB/README.md`**
- 参考项目说明：`readme.md`（原作者的拓展坞 v2 记录）
