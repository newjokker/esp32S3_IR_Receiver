import serial
import time

def get_csv_from_esp32(port, baudrate=115200, timeout=5):
    try:
        with serial.Serial(port, baudrate, timeout=timeout) as ser:
            time.sleep(2)  # 等待连接稳定
            
            # 清空缓冲区
            ser.reset_input_buffer()
            ser.reset_output_buffer()
            
            # 发送命令
            ser.write(b"export\n")
            ser.flush()
            
            print("等待CSV数据...")
            csv_data = []
            start_marker = False
            
            start_time = time.time()
            while (time.time() - start_time) < timeout:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8').strip()
                    
                    if line == "CSV_START":
                        start_marker = True
                        continue
                    elif line == "CSV_END":
                        break
                    elif start_marker and line:  # 只收集标记之间的数据
                        # 过滤掉调试信息
                        if not line.startswith(("按钮按下", "数据已保存", "ERROR:")):
                            csv_data.append(line)
                
                time.sleep(0.01)
            
            if not csv_data:
                print("未收到有效数据")
                return None
            
            # 保存到文件
            with open('esp32_data.csv', 'w') as f:
                f.write("\n".join(csv_data))
            
            print(f"成功获取 {len(csv_data)} 行数据")
            return csv_data
            
    except Exception as e:
        print(f"错误: {e}")
        return None

if __name__ == "__main__":

    port = '/dev/cu.wchusbserial5A7B1617701'
    
    data = get_csv_from_esp32(port)
    if data:
        print("\nCSV内容:")
        for line in data:
            print(line)