# ESP32-S3 ODrive CAN Translator

Serial-to-CAN translator for **ODrive v0.5.1** motor controllers running on **XIAO ESP32-S3**.

## Overview

This project enables interactive control of ODrive motor drives over CAN bus via serial terminal. Perfect for development, testing, and robotics applications.

**Features:**
- ✅ 25 ODrive CAN commands (v0.5.1)
- ✅ 250 kbps CAN baudrate
- ✅ XIAO ESP32-S3 optimized
- ✅ Serial CLI interface
- ✅ FreeRTOS multitasking
- ✅ ESP-IDF v6.0.1

## Hardware Setup

### Board
- **XIAO ESP32-S3** (Seeed Studio) - [Docs](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)

### Wiring (On-chip CAN/TWAI)

**XIAO ESP32-S3 → ODrive v0.5.1:**

```
GPIO 21 (CAN TX)  ──→  ODrive CAN-H (Pin 1)
GPIO 20 (CAN RX)  ──→  ODrive CAN-L (Pin 2)
GND               ──→  ODrive GND (Pin 3)
```

### Pin Reference

| Function | GPIO | XIAO Pin | ODrive |
|----------|------|----------|--------|
| CAN TX | 21 | D5 | CAN-H |
| CAN RX | 20 | D4 | CAN-L |
| GND | GND | GND | GND |

**For MCP2515 SPI Module (alternative):**
- CS: GPIO 44
- MOSI: GPIO 9
- MISO: GPIO 8
- SCK: GPIO 7

→ See [HARDWARE_SETUP.md](HARDWARE_SETUP.md) for detailed diagrams

## ODrive Configuration

Before flashing, configure ODrive via USB:

```python
# Using odrivetool
odrv0.axis0.config.can_node_id = 0
odrv0.save_configuration()
odrv0.reboot()
```

## Build & Flash

### Prepare Environment

```bash
# Export ESP-IDF (adjust path if needed)
source ~/.espressif/v6.0.1/esp-idf/export.sh
```

### Set Target & Build

```bash
cd esp32s3-odrive-can-translator

# Set target to ESP32-S3
idf.py set-target esp32s3

# Configure (optional)
idf.py menuconfig

# Build
idf.py build
```

### Flash to Device

```bash
# Flash (adjust port as needed)
idf.py -p /dev/ttyACM0 flash

# Monitor output
idf.py -p /dev/ttyACM0 monitor
```

**Baud Rate:** 115200 (serial monitor)

## Serial Commands

Connect to serial port and type commands. Use `help` to see all options:

### Motion Commands

```
pos <position> <vel_ff> <torque_ff>
    Set target position (rotations)
    Example: pos 1.0 0 0

vel <velocity> <torque_ff>
    Set target velocity (rot/s)
    Example: vel 2.5 0

torque <torque>
    Set direct torque command (Nm)
    Example: torque 0.5
```

### Configuration Commands

```
mode <control_mode> <input_mode>
    Set control and input modes
    Example: mode 3 5  (position + trajectory)
    
state <axis_state>
    Set axis state
    Example: state 6  (closed loop)
    
vel_limit <limit>
    Set max velocity (rot/s)
    Example: vel_limit 10
```

### Diagnostic Commands

```
get_pos
    Query motor position and velocity
    
get_iq
    Query motor current (Iq)
    
get_voltage
    Query bus voltage
    
clear_err
    Clear error flags
    
estop
    Emergency stop
    
help
    Show all commands
```

## Control Modes Reference

**control_mode** values:
- 0: Voltage control
- 1: Torque control
- 2: Velocity control
- 3: **Position control** (common)

**input_mode** values:
- 0: Inactive
- 1: Direct passthrough
- 2: Velocity ramp
- 3: Position filter
- 4: Mix channels
- 5: **Trapezoid trajectory** (common)
- 6: Torque ramp

**axis_state** values:
- 1: Idle
- 2: Startup sequence
- 4: Calibrate anticogging
- 6: **Closed-loop control** (normal operation)
- 8: Encoder index search

## Typical Startup Sequence

```
> clear_err
Clear errors sent

> mode 3 5
Set modes: control=3, input=5

> state 6
Set axis state: 6

> pos 1.0 0 0
Set position: 1.00 rot

> get_pos
Encoder: pos=1.0123 rot, vel=0.0034 rot/s
```

## Troubleshooting

### "No CAN communication"
1. Check GPIO 21/20 connections
2. Verify ODrive `can_node_id = 0` setting
3. Add 120Ω terminator resistors on CAN bus

### Serial port not found
```bash
# List available ports
ls /dev/tty*
# Typical: /dev/ttyACM0, /dev/ttyUSB0
```

### Build fails
- Ensure ESP-IDF is properly sourced
- Run `idf.py fullclean` then rebuild
- Check IDF version (should be v6.0.1)

## Project Structure

```
esp32s3-odrive-can-translator/
├── CMakeLists.txt              # Root build config
├── README.md                   # This file
├── HARDWARE_SETUP.md           # Detailed pinout & wiring
├── sdkconfig                   # ESP-IDF configuration
└── main/
    ├── CMakeLists.txt          # Component config
    ├── main.c                  # Application entry
    ├── odrive_translator.h     # CAN message definitions
    └── odrive_translator.c     # CAN helpers
```

## References

- [XIAO ESP32-S3 Wiki](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [ODrive v0.5.1 Docs](https://docs.odriverobotics.com/v/develop/)
- [ODrive CAN Protocol](https://docs.odriverobotics.com/v/develop/api/remote-objects#canbus-protocol)
- [ESP-IDF TWAI](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/twai.html)

## License

This project is provided as-is for use with ODrive motor controllers.

