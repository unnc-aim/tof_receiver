#!/usr/bin/env python3
"""ROS2 节点：从 ESP32 串口读取 4 路 TOF 数据并发布为 tof_receiver/TOF 消息。"""
import json
import rclpy
from rclpy.node import Node
from tof_receiver.msg import Tof
import serial


class TofReceiverNode(Node):
    """订阅串口、解析 4 路 TOF 数据并以自定义消息发布的节点。"""

    def __init__(self):
        super().__init__('tof_receiver_node')

        # 1. 声明串口号参数和波特率参数（可在 launch 或命令行动态修改）
        self.declare_parameter('port', '/dev/ttyUSB0')
        self.declare_parameter('baud', 115200)

        port = self.get_parameter('port').value
        baud = self.get_parameter('baud').value

        # 2. 声明 ToF 话题（单条消息包含 4 条腿的高度和健康状态）
        self.pub_tof = self.create_publisher(Tof, '/tof', 10)

        # 3. 尝试打开物理串口
        #    timeout=0：read() 永不阻塞，配合 in_waiting + 行缓冲，
        #    避免被拆分的 USB 包导致 10ms 定时器回调阻塞最多 1 秒。
        #    exclusive=True：禁止 screen/minicom 等同时抢占同一端口。
        # 原版 timeout=1.0 有一定概率会阻塞实时性（实时传输时每次读会占 1s）：
        try:
            # self.ser = serial.Serial(port, baud, timeout=1.0)
            self.ser = serial.Serial(port, baud, timeout=0, exclusive=True)
            self.get_logger().info(
                f" [TOF_RECEIVER] Connected to ESP32 on {port}!")
        except Exception as e:
            self.get_logger().error(
                f" [TOF_RECEIVER] Failed to open serial port: {e}")
            raise e

        # 行缓冲：累积未以 \n 结尾的半行数据
        self._line_buffer = ''

        # 4. 创建 10ms 周期定时器（100Hz 频率高频接收并转发）
        self.timer = self.create_timer(0.01, self.read_serial_loop)

    def destroy_node(self):
        """关机前先释放串口，避免下次启动报 'device or resource busy'。"""
        try:
            if getattr(self, 'ser', None) is not None and self.ser.is_open:
                self.ser.close()
                self.get_logger().info(" [TOF_RECEIVER] Serial port closed.")
        except Exception as e:
            self.get_logger().error(
                f" [TOF_RECEIVER] Failed to close serial port: {e}")
        return super().destroy_node()

    def read_serial_loop(self):
        """非阻塞读取串口，按换行符切出完整行交给 _process_line。"""
        # 把当前缓冲区里所有可用字节一次性读出
        waiting = self.ser.in_waiting
        if waiting > 0:
            try:
                # 累加到行缓冲，再按 \n 切出完整行
                self._line_buffer += self.ser.read(waiting).decode(
                    'utf-8', errors='replace')
                while '\n' in self._line_buffer:
                    line, self._line_buffer = self._line_buffer.split('\n', 1)
                    self._process_line(line)

            except serial.SerialException as e:
                self.get_logger().error(
                    f" [TOF_RECEIVER] SerialException occurred: {e}")

            except Exception as e:
                self.get_logger().error(
                    " [TOF_RECEIVER] Error occurred while processing serial "
                    f"data: {e}")

    def _process_line(self, line):
        """解析一行 JSON 文本并发布为 tof_receiver/TOF 消息。"""
        line = line.strip()
        if not line:
            return

        # 串口文本格式为 JSON，例如：
        # {"detected":3,"distances":[
        #   {"channel":0,"distance":165},
        #   {"channel":1,"distance":170},
        #   {"channel":2,"distance":168}]}
        # channel(0~3) 对应 tof1~tof4，数组中出现的通道视为健康(health=True)。
        try:
            data = json.loads(line)
        except json.JSONDecodeError as e:
            self.get_logger().warn(
                f" [TOF_RECEIVER] Failed to parse JSON: {e}, raw: '{line}'")
            return

        distances = data.get('distances', [])

        # 初始化 4 条腿的距离和健康状态；未在 distances 中出现的通道保持不健康
        heights = [0] * 4
        healths = [False] * 4

        for item in distances:
            channel = item.get('channel')
            distance = item.get('distance')
            if channel is None or distance is None:
                continue
            if not (0 <= channel < 4):
                self.get_logger().warn(
                    f" [TOF_RECEIVER] Channel out of range: {channel}")
                continue

            heights[channel] = distance
            healths[channel] = True

            # add health check of length for each leg
            if distance < 0 or distance > 2000:
                self.get_logger().warn(
                    f" [TOF_RECEIVER] Invalid height value for leg "
                    f"{channel + 1}: {distance} mm")
                healths[channel] = False

        # 5. 封装为 Tof 消息并发布
        msg = Tof()
        msg.tof1_health = healths[0]
        msg.tof1_height = heights[0]
        msg.tof2_health = healths[1]
        msg.tof2_height = heights[1]
        msg.tof3_health = healths[2]
        msg.tof3_height = heights[2]
        msg.tof4_health = healths[3]
        msg.tof4_height = heights[3]
        self.pub_tof.publish(msg)


def main(args=None):
    """节点入口：初始化、spin、并在退出时清理资源。"""
    rclpy.init(args=args)
    node = None
    try:
        node = TofReceiverNode()
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        # 构造函数抛异常时 node 仍为 None，跳过 destroy_node 但保证 shutdown
        if node is not None:
            node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
