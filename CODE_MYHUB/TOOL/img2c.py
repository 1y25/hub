# -*- coding: utf-8 -*-
"""
img2c.py — 动图帧图片 → STM32可烧录的 .c 数组文件

把动图分帧后的图片（PNG/JPG/BMP...）或 GIF 直接转换成
flashpic.py 能识别的 .c 文件（格式与 Image2LCD 输出一致）：

    头8字节: 0x01,0x10,宽低,宽高,高低,高高,0x01,0x1B
    数据:    RGB565, 低字节在前(小端), 每像素2字节

用法:
    # 1) 输入是"已经分好帧的图片目录"（文件名按数字排序, 从1开始）
    python img2c.py ./frames --out ./mygif
    python img2c.py ./frames --out ./mygif --size 130x130

    # 2) 输入是单个 GIF，自动拆帧
    python img2c.py ./anim.gif --out ./mygif --size 130x130

    # 3) 生成后再用 flashpic.py 烧录（第3个图库位置, 共N帧）
    python flashpic.py ./mygif 3 N

参数:
    --size WxH   强制缩放所有帧（推荐: 130x130 或 160x128, 别超过屏幕可视区域）
    --swap-rb    若屏幕颜色红蓝反了, 加上这个开关重新生成
    --bin        同时输出 1.bin 2.bin ... 原始二进制(方便自己核对/备用)

依赖:  pip install pillow
"""
import argparse
import os
import re
import sys

try:
    from PIL import Image
except ImportError:
    print("❌ 缺少 Pillow，请先运行:  pip install pillow")
    sys.exit(1)


def rgb888_to_565(r, g, b, swap_rb=False):
    """RGB888 -> RGB565（16位）"""
    if swap_rb:
        r, b = b, r
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def detect_content_bbox(img, bg):
    """检测非背景内容的最小包围盒, 背景色差异小于阈值视为背景"""
    px = img.load()
    w, h = img.size
    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y][:3]
            if abs(r - bg[0]) > 8 or abs(g - bg[1]) > 8 or abs(b - bg[2]) > 8:
                if x < minx: minx = x
                if x > maxx: maxx = x
                if y < miny: miny = y
                if y > maxy: maxy = y
    if maxx < 0:
        return None
    return (minx, miny, maxx, maxy)


def center_frames(frames, bg):
    """把所有帧的内容(包围盒并集)整体居中到画布, 动画移动时不跳动"""
    bboxes = [detect_content_bbox(img, bg) for img in frames]
    bboxes = [b for b in bboxes if b]
    if not bboxes:
        print("⚠️  检测不到内容, 跳过居中")
        return frames
    uminx = min(b[0] for b in bboxes)
    uminy = min(b[1] for b in bboxes)
    umaxx = max(b[2] for b in bboxes)
    umaxy = max(b[3] for b in bboxes)
    cw, ch = umaxx - uminx + 1, umaxy - uminy + 1
    tw, th = frames[0].size
    ox = (tw - cw) // 2 - uminx
    oy = (th - ch) // 2 - uminy
    out = []
    for img in frames:
        canvas = Image.new("RGB", img.size, bg)
        canvas.paste(img, (ox, oy))
        out.append(canvas)
    print(f"🎯 自动居中: 内容 {cw}x{ch}, 整体偏移 ({ox},{oy})")
    return out


def image_to_c_bytes(img, swap_rb=False):
    """把 PIL 图像转成 [头8字节 + RGB565小端数据] 的 bytes"""
    w, h = img.size
    # 8字节头: 与 Image2LCD/参考项目一致
    header = bytes([0x01, 0x10, w & 0xFF, (w >> 8) & 0xFF,
                    h & 0xFF, (h >> 8) & 0xFF, 0x01, 0x1B])
    data = bytearray()
    # 按行从左到右、从上到下扫描（与参考项目像素顺序一致）
    px = img.load()
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y][:3]
            v = rgb888_to_565(r, g, b, swap_rb)
            data += bytes([v & 0xFF, (v >> 8) & 0xFF])  # 小端
    return header + bytes(data)


def write_c_file(path, idx, raw):
    total = len(raw)
    line = f"const unsigned char gImage_{idx}[{total}] = {{ "
    values = [f"0X{v:02X}" for v in raw]
    for i in range(0, len(values), 8):
        line += ",".join(values[i:i + 8]) + ",\n"
    line = line.rstrip(",\n") + "\n};\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(line)


def list_frames(src):
    """目录下按文件名中的数字排序的图片列表"""
    exts = (".png", ".jpg", ".jpeg", ".bmp", ".webp")
    files = []
    for name in os.listdir(src):
        low = name.lower()
        if not low.endswith(exts):
            continue
        m = re.search(r"(\d+)", name)
        idx = int(m.group(1)) if m else float("inf")
        files.append((idx, os.path.join(src, name)))
    files.sort(key=lambda x: x[0])
    return [f for _, f in files]


def main():
    parser = argparse.ArgumentParser(description="动图帧 → .c 数组文件")
    parser.add_argument("src", help="帧图片目录 或 单个GIF文件")
    parser.add_argument("--out", default="./out_c", help="输出目录(默认 ./out_c)")
    parser.add_argument("--size", default=None, help="强制缩放尺寸, 如 130x130")
    parser.add_argument("--swap-rb", action="store_true", help="红蓝交换(屏幕颜色反了时用)")
    parser.add_argument("--bin", action="store_true", help="同时输出 .bin 原始数据")
    parser.add_argument("--frames", type=int, default=None,
                        help="最多保留多少帧(超出时均匀抽帧, 默认全部保留)")
    parser.add_argument("--autocenter", action="store_true",
                        help="自动检测内容包围盒并居中到画布(去偏边)")
    args = parser.parse_args()

    size = None
    if args.size:
        m = re.match(r"^(\d+)[xX](\d+)$", args.size)
        if not m:
            print("❌ --size 格式错误, 应为 宽x高 例如 130x130")
            sys.exit(1)
        size = (int(m.group(1)), int(m.group(2)))

    # 收集帧(PIL Image对象列表)
    if os.path.isdir(args.src):
        paths = list_frames(args.src)
        if not paths:
            print(f"❌ 目录 {args.src} 里没有图片")
            sys.exit(1)
        frames = [Image.open(p).convert("RGB") for p in paths]
    elif args.src.lower().endswith(".gif"):
        gif = Image.open(args.src)
        frames = []
        try:
            idx = 0
            while True:
                gif.seek(idx)
                # 透明像素合成到白色背景, 避免透明变黑/残影重叠
                rgba = gif.convert("RGBA")
                bg = Image.new("RGBA", gif.size, (255, 255, 255, 255))
                bg.paste(rgba, (0, 0), rgba)
                frames.append(bg.convert("RGB"))
                idx += 1
        except EOFError:
            pass
        print(f"🎞  GIF 共拆出 {len(frames)} 帧")
    else:
        print("⚠️  输入既不是目录也不是GIF, 按单张图片处理")
        frames = [Image.open(args.src).convert("RGB")]

    # 超出帧数上限时均匀抽帧
    if args.frames and len(frames) > args.frames:
        total = len(frames)
        step = total / args.frames
        frames = [frames[int(i * step)] for i in range(args.frames)]
        print(f"🎬 原 {total} 帧, 均匀抽帧到 {args.frames} 帧")

    os.makedirs(args.out, exist_ok=True)

    # 统一缩放到目标尺寸
    if size:
        frames = [img.resize(size, Image.LANCZOS) for img in frames]
    # 自动居中(去偏边)
    if args.autocenter and frames:
        bg_color = frames[0].getpixel((0, 0))   # 第一帧左上角作为背景色
        frames = center_frames(frames, bg_color)

    total_c = 0
    for i, img in enumerate(frames, start=1):
        raw = image_to_c_bytes(img, args.swap_rb)
        w, h = img.size
        if len(raw) - 8 != w * h * 2:
            print(f"❌ 第{i}帧尺寸异常")
            sys.exit(1)

        c_path = os.path.join(args.out, f"{i}.c")
        write_c_file(c_path, i, raw)
        if args.bin:
            with open(os.path.join(args.out, f"{i}.bin"), "wb") as f:
                f.write(raw)
        total_c += 1
        print(f"✅ 第{i}帧: {w}x{h} -> {c_path} ({len(raw)}字节)")

    print(f"\n🎉 完成, 共生成 {total_c} 个 .c 文件到 {args.out}")
    print(f"   接着执行: python flashpic.py {args.out} <图库下标1-4> {total_c}")


if __name__ == "__main__":
    main()
