import serial
import time
import re
import sys
import os
import argparse

# ================== 配置 ==================
PORT = "COM24"          # 改成你的串口
LOW_BAUD = 9600
HIGH_BAUD = 115200
PACKET_SIZE = 2048


# ================== 解析C数组 ==================
def parse_c_array(file_path):
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # 提取所有 0xXX（支持0x/0X）
    hex_values = re.findall(r'0[xX]([0-9A-Fa-f]{1,2})', content)

    if len(hex_values) < 8:
        print("❌ 数据长度不足，无法解析头")
        sys.exit(1)

    raw = bytes(int(h, 16) for h in hex_values)

    # ========= 解析头 =========
    header = raw[:8]

    width  = header[2] | (header[3] << 8)
    height = header[4] | (header[5] << 8)

    print(f"📐 图像尺寸: {width} x {height}")

    # ========= 去掉头 =========
    data = raw[8:]

    print(f"✅ 有效数据: {len(data)} 字节")

    return data, width, height


# ================== 格式化命令 ==================
def format_cmd(packet_count, width, height, a, b):
    return "WritePic-{xxx}-{yyy}-{zzz}-{aaa}-{bbb}\r\n".format(
        xxx=f"{packet_count:03d}",
        yyy=f"{width:03d}",
        zzz=f"{height:03d}",
        aaa=f"{a:03d}",
        bbb=f"{b:03d}",
    )


# ================== 主流程 ==================
def main():
    if __name__ == "__main__":

        parser = argparse.ArgumentParser()
        parser.add_argument(
            "folder",
            help="图片c文件目录"
        )

        parser.add_argument(
            "index",
            type=int,
            help="图片下标"
        )

        parser.add_argument(
            "count",
            type=int,
            help="图片数量"
        )

        args = parser.parse_args()


        batch_send(
            args.folder,
            args.index,
            args.count
        )

        
        time.sleep(3)
        ser = serial.Serial(PORT, LOW_BAUD, timeout=5)
        print("图片烧录结束，重启单片机")
        cmd = "RESET"
        ser.write(cmd.encode())
        ser.close()




def send_image(file_path,user_a,user_b):
    # 解析数据
    data, width, height = parse_c_array(file_path)

    # 获取宽高
    print(f"📐 图像尺寸: {width}x{height}")


    # 计算包数
    packet_count = (len(data) + PACKET_SIZE - 1) // PACKET_SIZE

    # 生成命令
    cmd = format_cmd(packet_count, width, height, user_a, user_b)

    # ================== 第一阶段：9600握手 ==================
    ser = serial.Serial(PORT, LOW_BAUD, timeout=5)
    time.sleep(1)

    print("📤 发送:", cmd.strip())
    ser.write(cmd.encode())

    resp = ser.readline().decode(errors='ignore').strip()
    print("📥 收到:", resp)

    if resp != "ReadyToReadPic":
        print("❌ MCU未准备好")
        ser.close()
        return

    # ================== 切换波特率 ==================
    ser.close()
    time.sleep(0.3)

    ser = serial.Serial(PORT, HIGH_BAUD, timeout=5)
    time.sleep(0.5)

    # 清缓存
    ser.reset_input_buffer()

    # ================== 等待第一个 Next ==================
    print("⏳ 等待 MCU Next...")

    while True:
        resp = ser.readline().decode(errors='ignore').strip()
        if resp:
            print("📥 收到:", resp)

        if resp.upper() == "NEXT":
            break

    # ================== 开始发送 ==================
    offset = 0
    index = 0

    while offset < len(data):
        chunk = data[offset:offset + PACKET_SIZE]

        # 发包
        ser.write(chunk)
        print(f"📦 发送包 {index+1}/{packet_count} ({len(chunk)} 字节)")

        # 等 MCU 回 NEXT / Finish
        while True:
            resp = ser.readline().decode(errors='ignore').strip()

            if resp:
                print("📥 收到:", resp)

            if resp.upper() == "NEXT":
                break

            elif resp.upper() == "FINISH":
                print("🎉 MCU结束")
                ser.close()
                return

        offset += PACKET_SIZE
        index += 1

    # ================== 等待 Finish ==================
    print("⏳ 等待 Finish...")

    while True:
        resp = ser.readline().decode(errors='ignore').strip()

        if resp:
            print("📥 收到:", resp)

        if resp.upper() == "FINISH":
            print("✅ 传输完成")
            break

    ser.close()



def batch_send(folder, pic_index, frame_count):

    for frame in range(1, frame_count + 1):

        file_path = os.path.join(
            folder,
            f"{frame}.c"
        )

        if not os.path.exists(file_path):
            print(f"❌ 找不到 {file_path}")
            continue


        print("\n====================")
        print(f"开始发送第 {frame} 帧")
        print("====================")


        # 用户变量
        user_a = pic_index
        user_b = frame


        send_image(
            file_path,
            user_a,
            user_b
        )


        print(
            f"✅ 第 {frame} 帧完成"
        )


        time.sleep(1)

# ================== 入口 ==================
if __name__ == "__main__":
    main()
