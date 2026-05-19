# ODrive 0.5.1 Serial-to-CAN Translator Setup Guide

This guide documents the available CAN commands in ODrive firmware v0.5.1 and provides a complete template for implementing a serial-to-CAN translator on your ESP32-S3 with CAN expansion hat.

## 📋 Documentation Files Created

### 1. **SERIAL_TO_CAN_CONVERTER.md** (This folder)
Comprehensive protocol specification including:
- CAN frame format and ID calculation
- Complete message reference (write + RTR queries)
- Data type encoding (little-endian floats, scaled integers)
- Typical usage sequences
- Troubleshooting guide

### 2. **Repository Memory** (`/memories/repo/odrive-051-can-commands.md`)
Command reference with:
- Frame ID structure (6-bit node_id + 5-bit cmd_id)
- All 25 implemented messages
- Query (RTR) vs write semantics
- Known limitations for your output encoder use case

### 3. **Source Code** (Firmware directory)
- `odrive_translator.h` - Header with message enums and data packing utilities
- `odrive_translator.c` - CAN message sender functions
- `main_example.c` - Complete ESP32-S3 example with:
  - CAN driver initialization
  - Serial command parser
  - RX message queue handler
  - Example initialization sequence

---

## 🎯 Key Points: What Works & What Doesn't

### ✅ FULLY SUPPORTED (ODrive v0.5.1)

| Feature | Status | Notes |
|---------|--------|-------|
| Position control via CAN | ✅ | `Set_Input_Pos` (0x00C) with velocity/torque feedforward |
| Velocity control via CAN | ✅ | `Set_Input_Vel` (0x00D) |
| Torque control via CAN | ✅ | `Set_Input_Torque` (0x00E) |
| State management | ✅ | Set/monitor axis state (idle, closed-loop, etc.) |
| Error monitoring | ✅ | Query motor, encoder, sensorless errors |
| Current feedback | ✅ | `Get_Iq` (0x014) returns setpoint + measured |
| Bus voltage monitoring | ✅ | `Get_Vbus_Voltage` (0x017) |
| Cyclic heartbeat | ✅ | 100ms update rate with error/state info |
| Velocity limits | ✅ | `Set_Vel_Limit` (0x00F) |
| Trajectory planning | ✅ | `Set_Traj_*` commands for accel/decel |
| Motor encoder position | ✅ | `Get_Encoder_Estimates` (0x009) **MOTOR ONLY** |

### ❌ NOT AVAILABLE (ODrive v0.5.1)

| Feature | Status | Workaround |
|---------|--------|-----------|
| Output encoder position via CAN | ❌ | Read absolute encoder separately (GPIO/SPI) |
| Firmware version query | ❌ | Hard-code v0.5.1 detection (send test command) |
| Set absolute position homing | ❌ | Manual USB/odrivetool setup before CAN use |
| Cyclic encoder updates | ❌ | Use RTR queries but adds latency per cycle |
| Temperature monitoring | ❌ | Not implemented in CANSimple for v0.5.1 |
| Current limit setting | ❌ | Use velocity/position limits instead |

### ⚠️ WORST CASE FOR YOUR ROBOT ARM

**Your absolute encoder on the joint output CANNOT be read via CAN.**
- `Get_Encoder_Estimates` only returns motor shaft position
- Load/output encoder data is not available over the CAN bus
- Your ROS2 hardware interface will always report motor angle, not joint angle
- **Solution**: Either:
  1. Move absolute encoder to motor shaft (breaks your design goal)
  2. Read absolute encoder separately via GPIO/SPI to ESP32-S3
  3. Request firmware upgrade to ODrive 0.5.6+ (zauberzeug fork has load encoder CAN support)

---

## 🚀 Quick Start: Flashing ESP32-S3

### Prerequisites
1. **ESP-IDF** installed and configured in VS Code
2. **MCP2515 CAN hat** connected to ESP32-S3:
   - Pin mapping (customize for your hat):
     - TX_GPIO_NUM = 41
     - RX_GPIO_NUM = 40
     - GND, VCC, CAN_H, CAN_L connections
3. **ODrive configured** via USB:
   ```
   odrv0.axis0.config.can_node_id = 0
   odrv0.save_configuration()
   ```

### Steps

1. **Create ESP-IDF project structure** (if not done):
   ```bash
   cd ~/esp-projects
   idf.py create-project odrive-translator
   cd odrive-translator
   ```

2. **Copy source files**:
   - Copy `odrive_translator.h` → `main/`
   - Copy `odrive_translator.c` → `main/`
   - Copy `main_example.c` → `main/main.c` (or merge into existing)
   - Review `CMakeLists.txt` in `main/` folder

3. **Configure CAN GPIO pins** in `main/main.c`:
   ```c
   #define TX_GPIO_NUM 41  // Adjust for your hat
   #define RX_GPIO_NUM 40  // Adjust for your hat
   ```

4. **Build & Flash**:
   ```bash
   idf.py build
   idf.py -p /dev/ttyACM0 flash monitor
   ```

5. **Test in Serial Monitor**:
   ```
   > help                    # Show all commands
   > get_voltage             # Query bus voltage
   > mode 3 5                # Set position+trajectory mode
   > state 6                 # Enter closed-loop
   > pos 1.0 0 0             # Move to 1.0 rotation
   > get_pos                 # Query current position
   ```

---

## 📡 Available Serial Commands

### Motion Control
```
pos <position> <vel_ff> <torque_ff>   # Set position (rotations)
vel <velocity> <torque_ff>             # Set velocity (rot/s)
torque <torque>                        # Set torque (Nm)
vel_limit <limit>                      # Set max velocity (rot/s)
```

### Configuration
```
mode <control_mode> <input_mode>       # Set control method
state <state>                          # Request axis state (6=closed_loop)
```

### Diagnostics
```
get_pos                                # Query motor position + velocity
get_iq                                 # Query current setpoint + measured
get_voltage                            # Query bus voltage
clear_err                              # Clear error flags
estop                                  # Emergency stop
```

### Help
```
help                                   # Show all commands
```

---

## 🔌 CAN Message Format Reference

### Position Command (0x00C)
```
CAN ID: (node_id << 5) | 12 = 0x00C (for node_id=0)
DLC: 8 bytes

Byte 0-3:  float32   = position (rotations)
Byte 4-5:  int16     = velocity feedforward / 0.001
Byte 6-7:  int16     = torque feedforward / 0.001

Example: Move to 2.5 rotations, no feedforward
Bytes: 0x00 0x00 0x20 0x40 | 0x00 0x00 | 0x00 0x00
       └─ 2.5 (float) │       └─ 0 ─┘  └─ 0 ─┘
```

### Heartbeat (Auto, 0x001)
```
CAN ID: 0x001
DLC: 8 bytes (sent every 100ms)

Byte 0-3:  uint32  = axis error flags
Byte 4-7:  uint32  = axis state (0=idle, 6=closed_loop)

Parse in your code:
uint32_t error = unpack_int32_le(msg->data, 0);
uint32_t state = unpack_int32_le(msg->data, 4);
```

### Encoder Query (RTR, 0x009)
```
Request:  CAN ID 0x009, RTR=1, DLC=0
Response: CAN ID 0x009, RTR=0, DLC=8

Byte 0-3:  float32  = position estimate (rotations) [MOTOR ONLY]
Byte 4-7:  float32  = velocity estimate (rot/s)

Parse response:
float pos = unpack_float32_le(msg->data, 0);
float vel = unpack_float32_le(msg->data, 4);
```

---

## 🔧 Troubleshooting

| Problem | Cause | Solution |
|---------|-------|----------|
| No CAN messages from ODrive | Node ID mismatch | Check `can_node_id = 0` via USB/odrivetool |
| "CAN transmit failed" | Bus error or wrong GPIO | Verify CAN pins and connections (CAN_H/L) |
| Commands ignored | Motor not in closed-loop | Send `state 6` first, check heartbeat |
| RTR query timeout | Watchdog timeout | ODrive needs continuous heartbeat; check connection |
| Garbled position values | Endianness error | Verify little-endian packing in `pack_float32_le()` |
| Serial commands not recognized | Typo in command | Check spacing; use `help` command |

---

## 📚 File Structure

```
Firmware/
├── SERIAL_TO_CAN_CONVERTER.md    # This guide
├── odrive_translator.h            # Header with enums & packing functions
├── odrive_translator.c            # CAN sender implementations
├── main_example.c                 # Complete ESP32-S3 example
│
└── [existing ODrive files]
    ├── communication/can_simple.cpp
    ├── communication/can_simple.hpp
    └── ...
```

---

## 📖 Reference Documentation

- **OD v0.5.1 CAN Source**: See [Firmware/communication/can_simple.cpp](communication/can_simple.cpp)
- **Message Definitions**: See [Firmware/communication/can_simple.hpp](communication/can_simple.hpp) (MSG_* enums)
- **Full Command Reference**: See `SERIAL_TO_CAN_CONVERTER.md` (this directory)
- **Implementation Details**: See repository memory `/memories/repo/odrive-051-can-commands.md`

---

## ⚠️ Important Reminders

1. **Output Encoder Not Available**: You MUST read your absolute joint encoder separately
   - Via GPIO interrupt or SPI to ESP32-S3
   - NOT through ODrive CAN bus

2. **No Version Query**: Hard-code firmware detection for v0.5.1
   - Consider adding a fallback test message to verify connection

3. **RTR Adds Latency**: Every position query requires a request-response roundtrip
   - If you need 1 kHz control, this won't work (too slow)
   - Good for 50-100 Hz monitoring

4. **Watchdog Feeds Automatically**: Any CAN message resets the watchdog
   - If your ESP32 crashes, ODrive will time out and stop

5. **Little-Endian Encoding**: All multi-byte values use Intel byte order
   - Float32 is transmitted LSB first
   - Test with known values first (e.g., velocity limit of 5.0)

---

## 🎯 Next Steps

1. **Verify CAN Connection**:
   ```bash
   idf.py build
   idf.py -p /dev/ttyACM0 flash
   idf.py monitor
   # Should see "Heartbeat - Errors: 0x00000000, State: 1" every 100ms
   ```

2. **Test Basic Motion**:
   ```
   > mode 3 5         # Position + Trajectory
   > state 6          # Close loop
   > pos 1.0 0 0      # Move
   > get_pos          # Verify
   ```

3. **Integrate Absolute Encoder**:
   - Read joint encoder on GPIO/SPI in parallel task
   - Combine motor position (from ODrive) + joint offset in your ROS2 hardware interface

4. **Build Your Control Loop**:
   - Monitor heartbeat for errors
   - Query position/current as needed from ROS2
   - Send position setpoints at your desired control rate

---

**Created**: May 17, 2026  
**For**: ODrive Firmware v0.5.1 on MKS Mini  
**Platform**: ESP32-S3 with MCP2515 CAN Hat  
**Author**: GitHub Copilot
