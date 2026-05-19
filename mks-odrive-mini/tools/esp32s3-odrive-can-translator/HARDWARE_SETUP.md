# ESP32-S3 ODrive CAN Translator - Hardware Setup

This project implements a serial-to-CAN translator for ODrive motor controllers using ESP32-S3.

## Supported Boards

### XIAO ESP32-S3 Board

The primary board for this project is the **XIAO ESP32-S3** (Seeed Studio).

#### On-chip TWAI (CAN) Configuration

For on-chip CAN communication, use these GPIO pins:

```
XIAO ESP32-S3 Pin Layout:
┌─────────────────────────────────────────┐
│  D0/GND  D1   D2   D3   D4   D5  D6/GND │  IO Row 1
│  3.3V    5V   GND  GND  GND  USB       │
│  D10     D9   D8   D7  3V3            │  IO Row 2
│  D9(MOSI)D8(MISO)D7(SCK)D6(CS)        │  SPI Row
└─────────────────────────────────────────┘

CAN (TWAI) Pins:
- TX: GPIO 21 (connect to ODrive CAN-H / CAN_TX)
- RX: GPIO 20 (connect to ODrive CAN-L / CAN_RX)
- GND: Connect to ODrive GND (common ground)
```

**Baudrate:** 250 kbps (hardcoded to match ODrive default)

#### External MCP2515 via SPI (Alternative)

If using an external MCP2515 CAN module connected via SPI:

```
MCP2515 Module Pin → XIAO ESP32-S3 Pin:
- CS   → GPIO 44 (Chip Select)
- MOSI → GPIO 9  (Master Out, Slave In)
- MISO → GPIO 8  (Master In, Slave Out)
- SCK  → GPIO 7  (Serial Clock)
- INT  → GPIO (optional, for interrupts)
- GND  → GND
- VCC  → 3.3V
```

This configuration is not currently implemented in the firmware, but the pins are documented for future reference.

## Wiring Example: ODrive v0.5.1

Connect to ODrive's CAN port (typically a 4-pin connector):

```
XIAO ESP32-S3          ODrive v0.5.1
─────────────────      ──────────────
GPIO 21 (CAN TX)  →    CAN-H (pin 1)
GPIO 20 (CAN RX)  →    CAN-L (pin 2)
GND               →    GND (pin 3)
─────────────────      (pin 4 unused)
```

**Important:** Use 120Ω termination resistors on the CAN bus if not already present in your ODrive setup.

## ODrive Configuration

Before using the translator, configure your ODrive to use CAN:

```python
# Via USB connection (using odrivetool):
odrv0.axis0.config.can_node_id = 0          # Must match translator
odrv0.save_configuration()
odrv0.reboot()
```

## Build & Flash

### Using ESP-IDF

```bash
cd esp32s3-odrive-can-translator

# Set target
source ~/.espressif/v6.0.1/esp-idf/export.sh
idf.py set-target esp32s3

# Configure pins (if different from defaults)
idf.py menuconfig
# Navigate to: Component config → TWAI Driver

# Build
idf.py build

# Flash to device (adjust /dev/ttyACM0 to your serial port)
idf.py -p /dev/ttyACM0 flash

# Monitor serial output
idf.py -p /dev/ttyACM0 monitor
```

## Serial Commands

Once flashed and connected, send commands via the serial terminal (115200 baud):

```
help                           # Show all available commands
pos <pos> <vel_ff> <torque_ff> # Set position (rotations)
vel <velocity> <torque_ff>     # Set velocity (rot/s)
torque <torque>                # Set torque (Nm)
mode <ctrl_mode> <input_mode>  # Set control mode
state <state>                  # Set axis state (6=closed_loop)
vel_limit <limit>              # Set velocity limit
get_pos                        # Query encoder position
get_iq                         # Query motor current
get_voltage                    # Query bus voltage
estop                          # Emergency stop
clear_err                      # Clear error flags
```

### Example Session

```
> help
=== ODrive CAN Command Reference ===
pos <float> <float> <float>   - Set position, vel_ff, torque_ff
vel <float> <float>           - Set velocity, torque_ff
...

> clear_err
Clear errors sent

> state 6
Set axis state: 6

> pos 1.0 0 0
Set position: 1.00 rot, vel_ff: 0.000, torque_ff: 0.000
```

## Features

- ✅ Full CAN message support for ODrive v0.5.1
- ✅ 25 CAN command types implemented
- ✅ Serial CLI for easy command entry
- ✅ 250 kbps CAN baudrate (ODrive default)
- ✅ FreeRTOS multitasking
- ✅ Compiled for ESP32-S3 architecture

## Troubleshooting

### No CAN communication

1. Check GPIO pin connections
2. Verify ODrive `can_node_id = 0` configuration
3. Monitor CAN bus with a CAN analyzer to see frame activity
4. Check for 120Ω termination resistors on CAN bus

### Serial errors ("uart_read_bytes error")

Normal during idle periods - the serial input task waits for commands. Errors disappear when data is received.

### Flash fails

- Ensure XIAO ESP32-S3 is in bootloader mode (press reset button)
- Check serial port with `ls /dev/tty*`
- Verify baudrate matches (460800 for esptool)

## Pin Customization

To change CAN pins, edit [main/main.c](main/main.c) and update:

```c
#define TX_GPIO_NUM 21      // Change this
#define RX_GPIO_NUM 20      // Change this
```

Then rebuild and flash.

## References

- [XIAO ESP32-S3 Datasheet](https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/)
- [ODrive v0.5.1 CAN Protocol](https://docs.odriverobotics.com/v/develop/api/remote-objects#canbus-protocol)
- [ESP-IDF TWAI Driver](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/twai.html)

