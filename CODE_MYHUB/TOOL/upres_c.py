# -*- coding: utf-8 -*-
"""
upres_c.py — 把已生成的 .c 帧文件(如130x130)直接放大/居中转成 160x128 全屏

场景: 旧动图的源文件(GIF/图片)找不到了, 只剩 img2c 生成的 1.c 2.c ...
这个脚本读取 .c 帧 → 解析RGB565 → 缩放到 160x128 → 重新输出 .c

用法:
    python upres_c.py ./mygif --out ./mygif_full
    python upres_c.py ./mygif --out ./mygif_full --size 160x128 --swap-rb
参数:
    --size WxH   目标尺寸(默认 160x128)
    --swap-rb    红蓝交换(颜色反了时用)
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

DEFAULT_SIZE = (160, 128)


def parse_c_file(path):
    """读取 .c 文件, 返回 (width, height, rgb888像素列表)"""
    with open(path, "r", encoding="utf-8") as f:
        content = f.read()
    hex_values = re.findall(r'0[xX]([0-9A-Fa-f]{1,2})', content)
    if len(hex_values) < 8:
        print(f"❌ {path}: 数据不足")
        sys.exit(1)
    raw = bytes(int(h, 16) for h in hex_values)
    w = raw[2] | (raw[3] << 8)
    h = raw[4] | (raw[5] << 8)
    if len(raw) - 8 != w * h * 2:
        print(f"❌ {path}: 尺寸 {w}x{h} 与数据长度不符 ({len(raw)-8} != {w*h*2})")
        sys.exit(1)
    # RGB565 小端 -> RGB888
    px = []
    for i in range(8, len(raw), 2):
        v = raw[i] | (raw[i + 1] << 8)
        r = (v >> 11) & 0x1F
        g = (v >> 5) & 0x3F
        b = v & 0x1F
        px.append((r << 3, g << 2, b << 3))
    return w, h, px


def rgb888_to_565(r, g, b, swap_rb=False):
    if swap_rb:
        r, b = b, r
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def write_c_file(path, idx, raw):
    total = len(raw)
    values = [f"0X{v:02X}" for v in raw]
    lines = []
    head = f"const unsigned char gImage_{idx}[{total}] = {{ "
    lines.append(head + ",".join(values[:8]))
    for i in range(8, len(values), 8):
        lines.append(",".join(values[i:i + 8]))
    text = ",\n".join(lines) + "\n};\n"
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


def main():
    parser = argparse.ArgumentParser(description="旧 .c 帧 → 160x128 全屏 .c")
    parser.add_argument("src", help="含 1.c 2.c ... 的目录")
    parser.add_argument("--out", default="./out_full", help="输出目录(默认 ./out_full)")
    parser.add_argument("--size", default=None, help="目标尺寸, 如 160x128 (默认 160x128)")
    parser.add_argument("--swap-rb", action="store_true", help="红蓝交换")
    args = parser.parse_args()

    if args.size:
        m = re.match(r"^(\d+)[xX](\d+)$", args.size)
        if not m:
            print("❌ --size 格式错误")
            sys.exit(1)
        target = (int(m.group(1)), int(m.group(2)))
    else:
        target = DEFAULT_SIZE

    # 找 1.c 2.c ... 连续文件
    files = []
    i = 1
    while True:
        p = os.path.join(args.src, f"{i}.c")
        if os.path.exists(p):
            files.append((i, p))
            i += 1
        else:
            break
    if not files:
        print(f"❌ 目录 {args.src} 里没有 1.c 2.c ... 帧文件")
        sys.exit(1)

    os.makedirs(args.out, exist_ok=True)
    for idx, path in files:
        w, h, px = parse_c_file(path)
        img = Image.new("RGB", (w, h))
        img.putdata(px)
        # 缩放到目标(保持比例, 居中黑边) — 动画帧通常直接拉伸填满
        img = img.resize(target, Image.LANCZOS)
        tw, th = target
        # 头8字节
        header = bytes([0x01, 0x10, tw & 0xFF, (tw >> 8) & 0xFF,
                        th & 0xFF, (th >> 8) & 0xFF, 0x01, 0x1B])
        data = bytearray()
        px2 = img.load()
        for y in range(th):
            for x in range(tw):
                r, g, b = px2[x, y][:3]
                v = rgb888_to_565(r, g, b, args.swap_rb)
                data += bytes([v & 0xFF, (v >> 8) & 0xFF])
        out_path = os.path.join(args.out, f"{idx}.c")
        write_c_file(out_path, idx, header + bytes(data))
        print(f"✅ 第{idx}帧: {w}x{h} -> {tw}x{th} -> {out_path}")

    print(f"\n🎉 完成, 共 {len(files)} 帧, 输出到 {args.out}")
    print(f"   接着: python flashpic.py {args.out} <图库下标1-4> {len(files)}")


if __name__ == "__main__":
    main()
