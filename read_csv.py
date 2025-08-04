import serial
import time

def wait_for_ready(ser, timeout=3):
    ser.reset_input_buffer()
    ser.write(b"ping\n")

    start = time.time()
    while time.time() - start < timeout:
        if ser.in_waiting:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            print(f"[ESP32] {line}")
            if "OK:PONG" in line:
                return True
        time.sleep(0.05)
    return False


def get_csv_from_esp32(port, baudrate=115200, timeout=10):
    try:
        with serial.Serial(port, baudrate, timeout=1) as ser:
            if not wait_for_ready(ser):
                print("ESP32 未就绪")
                return None

            # 清空缓冲
            ser.reset_input_buffer()
            ser.write(b"export\n")

            print("[INFO] 请求导出CSV...")
            start_time = time.time()
            csv_data = []
            collecting = False

            while time.time() - start_time < timeout:
                if ser.in_waiting:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if not line:
                        continue
                    print(f"[DATA] {line}")

                    if line == "CSV_START":
                        collecting = True
                        continue
                    elif line == "CSV_END":
                        break
                    elif collecting:
                        csv_data.append(line)

                time.sleep(0.01)

            if not csv_data:
                print("[ERROR] 没有收到数据")
                return None

            with open("esp32_log.csv", "w") as f:
                f.write("\n".join(csv_data))

            print(f"[OK] 成功保存 {len(csv_data)} 行数据到 esp32_log.csv")
            return csv_data

    except Exception as e:
        print(f"[ERROR] {e}")
        return None

if __name__ == "__main__":
    port = "/dev/cu.wchusbserial5A7B1617701" 
    data = get_csv_from_esp32(port)
    if data:
        print("\n=== CSV 内容预览 ===")
        for line in data:
            print(line)
