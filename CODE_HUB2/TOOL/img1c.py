# -*- coding: utf-8 -*-
"""
img1c.py — 图片/GIF → 屏幕 .c 帧文件（普通版, 不做自动居中）

和 img2c.py 的区别: 没有 --autocenter, 图片原样按 --size 缩放, 内容偏哪就显示哪。
GIF 透明像素会合成到白色背景(避免黑块)。

用法:
    # 图片或帧目录
    python img1c.py 图片.png --out ./out --size 160x128
    python img1c.py ./帧目录 --out ./out --size 160x128
    # GIF(自动拆帧, 超出帧数自动抽帧)
    python img1c.py anim.gif --out ./out --size 160x128 --frames 25

参数:
    --size WxH   目标尺寸(默认保持原尺寸)
    --frames N   最多保留N帧(超出均匀抽帧)
    --swap-rb    红蓝交换(颜色反了用)
    --bin        同时输出 .bin
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
    if swap_rb:
        r, b = b, r
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def image_to_c_bytes(img, swap_rb=False):
    """PIL图像 -> [8字节头 + RGB565小端数据]"""
    w, h = img.size
    header = bytes([0x01, 0x10, w & 0xFF, (w >> 8) & 0xFF,
                    h & 0xFF, (h >> 8) & 0xFF, 0x01, 0x1B])
    data = bytearray()
    px = img.load()
    for y in range(h):
        for x in range(w):
            r, g, b = px[x, y][:3]
            v = rgb888_to_565(r, g, b, swap_rb)
            data += bytes([v & 0xFF, (v >> 8) & 0xFF])
    return header + bytes(data)


def write_c_file(path, idx, raw):
    total = len(raw)
    values = [f"0X{v:02X}" for v in raw]
    lines = []
    lines.append(f"const unsigned char gImage_{idx}[{total}] = {{ " + ",".join(values[:8]))
    for i in range(8, len(values), 8):
        lines.append(",".join(values[i:i + 8]))
    text = ",\n".join(lines) + "\n};\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def list_frames(src):
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
    parser = argparse.ArgumentParser(description="图片/GIF → .c 帧文件(普通版, 不居中)")
    parser.add_argument("src", help="图片文件 或 GIF 或 帧图片目录")
    parser.add_argument("--out", default="./out_c", help="输出目录(默认 ./out_c)")
    parser.add_argument("--size", default=None, help="目标尺寸, 如 160x128")
    parser.add_argument("--frames", type=int, default=None, help="最多保留N帧(超出均匀抽帧)")
    parser.add_argument("--swap-rb", action="store_true", help="红蓝交换")
    parser.add_argument("--bin", action="store_true", help="同时输出 .bin")
    args = parser.parse_args()

    size = None
    if args.size:
        m = re.match(r"^(\d+)[xX](\d+)$", args.size)
        if not m:
            print("❌ --size 格式错误, 应为 宽x高 例如 160x128")
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

    # 统一缩放到目标尺寸
    if size:
        frames = [img.resize(size, Image.LANCZOS) for img in frames]

    os.makedirs(args.out, exist_ok=True)

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
