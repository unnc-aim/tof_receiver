# tof_receiver

通过 ESP32 读取 4 路 VL53L0X ToF 传感器数据，经串口转发为 ROS2 消息 `tof_receiver/TOF`，供足端高度估计等下游模块订阅。

---

## 硬件

- **MCU**：ESP32（任意带 USB-UART 的型号）
- **ToF 传感器**：4 × VL53L0X（I²C，固定地址 `0x29`）
- **I²C 多路复用器**：TCA9548A（地址 `0x70`），用于把 4 片同地址的 VL53L0X 隔离到 4 个通道

### 接线

| ESP32 | TCA9548A | VL53L0X ×4 |
|-------|----------|------------|
| GPIO21 (SDA) | SDA | SDA（并联，挂在各通道总线） |
| GPIO22 (SCL) | SCL | SCL（同上） |
| 3V3 / GND    | VCC/GND | VCC/GND |

> VL53L0X 的 SDA/SCL 分别接到 TCA9548A 的 **CH0–CH3** 总线上，每路一片。ESP32 只与 TCA9548A 的主总线相连。

---

## 串口协议

ESP32 以文本行的方式输出，波特率 `115200`，每行格式：

```
d0,d1,d2,d3,h0,h1,h2,h3\n
```

| 字段 | 含义 |
|------|------|
| `d0`–`d3` | 4 路 ToF 距离（mm，`int16`） |
| `h0`–`h3` | 4 路健康状态（`0`/`1`） |

健康状态判定（固件侧）：`初始化成功 && 未发生超时 && 0 < 读数 < 8000mm`。任一不满足即置 `0`。

---

## ROS2 消息

`tof_receiver/TOF`（见 `msg/TOF.msg`）：

```text
bool tof1_health
int32 tof1_height
bool tof2_health
int32 tof2_height
bool tof3_health
int32 tof3_height
bool tof4_health
int32 tof4_height
```

消息包含 `std_msgs/Header header`，时间戳为主机完整接收到该串口帧的时间。

---

## 话题与参数

### 发布话题

| 话题 | 类型 | 说明 |
|------|------|------|
| `/tof` | `tof_receiver/TOF` | 单条消息含 4 路高度 + 4 路健康状态 |

### 参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `port` | `/dev/ttyUSB0` | 串口设备节点 |
| `baud` | `115200` | 串口波特率 |

---

## 编译与运行

在工作空间根目录：

```bash
colcon build --packages-select tof_receiver
source install/setup.bash

# 默认串口 /dev/ttyUSB0
ros2 run tof_receiver tof_receiver_node

# 指定串口与波特率
ros2 run tof_receiver tof_receiver_node --ros-args -p port:=/dev/ttyACM0 -p baud:=115200
```

或用 launch 文件启动（推荐，参数透传更方便）：

```bash
# 默认参数
ros2 launch tof_receiver tof_receiver.launch.py

# 指定串口与波特率
ros2 launch tof_receiver tof_receiver.launch.py port:=/dev/ttyACM0 baud:=115200
```

查看数据：

```bash
ros2 topic echo /tof
ros2 topic hz /tof
```

### 烧录固件

`firmware/tof_firmware/arduino.ino` 用 Arduino IDE 烧录到 ESP32（依赖 `Wire` 与 Pololu `VL53L0X` 库）。

---

## 设计要点

- **非阻塞串口**：节点以 `timeout=0` + `read(in_waiting)` + 行缓冲按 `\n` 成帧，避免 USB 拆包导致定时器回调阻塞 1s。
- **范围校验**：主机端额外对每路高度做 `0–2000mm` 校验，越界置 `health=False` 并告警。
- **资源释放**：`destroy_node()` 会关闭串口，避免重启时报 `device or resource busy`。
- **端口独占**：串口以 `exclusive=True` 打开，防止 `screen`/`minicom` 同时抢占。

---

## 已知 TODO

1. 固件 `setTimeout(500)` 在某些工况下会拖慢回传时序，考虑降到约 100ms。
2. 传感器初始化失败时无硬件告警（计划加蜂鸣器）。
