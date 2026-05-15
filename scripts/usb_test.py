import usb.core
import usb.util
import math
import time

# 查找设备（替换为你的 VID:PID）
dev = usb.core.find(idVendor=0x054C, idProduct=0x0CE6)

if dev is None:
    raise ValueError('Device not found')

# 找到 Vendor 接口（interface 4）
interface = 4
endpoint_out = None
endpoint_in = None

for cfg in dev:
    for intf in cfg:
        if intf.bInterfaceNumber == interface:
            for ep in intf:
                if usb.util.endpoint_direction(ep.bEndpointAddress) == usb.util.ENDPOINT_OUT:
                    endpoint_out = ep
                else:
                    endpoint_in = ep

# 发送 64 字节数据（正弦波形）
data = bytearray(64)

# 生成前 64 字节的波形数据
for i in range(64):
    # 使用正弦波：-127 -> 0 -> 127 -> 0 -> -127
    value = int(127 * math.sin(2 * math.pi * i / 64))
    # 转换为无符号字节 (0-255)
    data[i] = value & 0xFF

print(f"Starting to send {len(data)} bytes, 1000 times...")
success_count = 0
fail_count = 0

for i in range(1000):
    try:
        dev.write(endpoint_out.bEndpointAddress, data)
        success_count += 1
        if i % 100 == 0:
            print(f"Sent {i} packets...")
        # 添加小延时，避免发送太快
        time.sleep(0.01)  # 1ms 延时
    except Exception as e:
        fail_count += 1
        print(f"Error at packet {i}: {e}")
        break

print(f"Done! Success: {success_count}, Failed: {fail_count}")