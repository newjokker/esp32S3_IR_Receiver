import serial
import time
import sys
from typing import Optional, List

class ESP32Logger:
    
    def __init__(self, port: str, baudrate: int = 115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = None

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()

    def connect(self) -> bool:
        """连接串口设备"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            time.sleep(2)  # 等待连接稳定
            return True
        except Exception as e:
            print(f"[ERROR] 无法连接串口设备: {e}")
            return False

    def disconnect(self):
        """断开串口连接"""
        if self.ser and self.ser.is_open:
            self.ser.close()

    def send_command(self, command: str, expected_response: str = None, timeout: float = 3.0):
        """发送命令并检查预期响应"""
        if not self.ser or not self.ser.is_open:
            print("[ERROR] 串口未连接")
            return False

        self.ser.reset_input_buffer()
        self.ser.write(f"{command}\n".encode())

        start_time = time.time()
        while time.time() - start_time < timeout:
            if self.ser.in_waiting:
                line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    # print(f"[ESP32] {line}")
                    if expected_response and expected_response in line:
                        return True
                    # 如果不需要特定响应，只要收到任何响应就返回成功
                    if not expected_response:
                        return True
            time.sleep(0.05)

        print(f"[ERROR] 命令 '{command}' 超时或未收到预期响应")
        return False

    def wait_for_ready(self, timeout: float = 5.0) -> bool:
        """等待设备就绪"""
        return self.send_command("ping", "OK:PONG", timeout)

    def export_csv(self, timeout: float = 10.0):
        """导出CSV数据"""
        
        # 这边一直到收到 CSV_START 为止
        if not self.send_command("export", "CSV_START", timeout):
            return None

        csv_data = []
        start_time = time.time()
        buffer = ""

        while time.time() - start_time < timeout:
            # 读取所有可用数据
            while self.ser.in_waiting:
                buffer += self.ser.read(self.ser.in_waiting).decode('utf-8', errors='ignore')
            
            # 处理缓冲区中的完整行
            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if not line:
                    continue

                if "CSV_END" not in line:
                    csv_data.append(line)
                    print(f"[DEBUG] 收集到数据: {line}")
                else:
                    return csv_data

            time.sleep(0.01)

        # 检查是否收集到数据但未收到CSV_END
        if csv_data:
            print("[WARNING] 超时前收到数据但未收到结束标记")
            return csv_data
        
        print("[DEBUG] 缓冲区最后内容:", buffer)  # 调试输出
        return None
    
    def clear_data(self) -> bool:
        """清除设备上的历史数据"""
        return self.send_command("clear", "OK:CSV_CLEARED")

    def save_to_file(self, data: List[str], filename: str = "esp32_log.csv"):
        """将数据保存到文件"""
        try:
            with open(filename, "w", encoding="utf-8") as f:
                f.write("\n".join(data))
            print(f"[SUCCESS] 数据已保存到 {filename} (共 {len(data)} 行)")
            return True
        except Exception as e:
            print(f"[ERROR] 保存文件失败: {e}")
            return False

def print_menu():
    """打印菜单选项"""
    print("\n=== ESP32 数据记录器 ===")
    print("1.  导出CSV数据")
    print("2.  导出并保存到文件")
    print("3.  清除并验证是否清除")
    print("99. 退出")
    return input("请选择操作 (1-5): ").strip()

def main():
    
    if len(sys.argv) < 2:
        print("使用方法: python esp32_logger.py <串口设备>")
        print("示例: python esp32_logger.py /dev/cu.wchusbserial5A7B1617701")
        return

    port = sys.argv[1]
    
    with ESP32Logger(port) as logger:
        if not logger.wait_for_ready():
            print("无法与ESP32建立通信，请检查连接")
            return

        while True:
            choice = print_menu()

            if choice == "1":  # 导出CSV
                data = logger.export_csv()
                if data:
                    print("\n=== CSV数据预览 (前5行) ===")
                    for line in data[:5]:
                        print(line)
                    if len(data) > 5:
                        print(f"...(共 {len(data)} 行，省略 {len(data)-5} 行)")

            elif choice == "2":  # 导出并保存
                data = logger.export_csv()
                if data:
                    filename = input("输入保存文件名 (默认: esp32_log.csv): ").strip() or "esp32_log.csv"
                    logger.save_to_file(data, filename)

            elif choice == "3":  # 清除并验证
                if logger.clear_data():
                    print("清除成功，正在验证...")
                    data = logger.export_csv()
                    if data and len(data) <= 1:  # 只有标题行
                        print("验证成功: 数据已清除")
                    else:
                        print("验证失败: 数据未完全清除")

            elif choice == "99":  # 退出
                print("再见!")
                break

            else:
                print("无效选择，请重新输入")

if __name__ == "__main__":
    
    main()