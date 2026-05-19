# ESP32-S3 Serial-to-CAN Converter Implementation Guide

This document describes how to implement a serial protocol to translate commands from the ESP32-S3 serial monitor into CAN messages for your ODrive 0.5.1 board.

## Hardware Setup
- **ESP32-S3** with MCP2515 CAN hat
- **ODrive MKS Mini** (S-fw-v0.5.1)
- **Connection**: CAN_H/CAN_L between ESP32 hat and ODrive
- **Default Baud Rate**: 250 kbps CAN bus

## ODrive CAN Configuration (via USB/odrivetool)
```
odrv0.axis0.config.can_node_id = 0        # Node ID 0
odrv0.config.can_heartbeat_rate_ms = 100  # 100ms heartbeat
```

---

## Serial Protocol Design

### Format: Text Commands Over Serial

Use simple ASCII commands for easy testing in serial monitor.

#### Command Syntax
```
[command] [arg1] [arg2] [arg3]
```

Examples:
```
pos 0 1.5 0 0       # Set position to 1.5 revolutions, vel_fwd=0, torque_fwd=0
vel 5.0 0           # Set velocity to 5 rot/s, torque_fwd=0
torque 2.5          # Set torque setpoint to 2.5 Nm
state 6             # Request state 6 (CLOSED_LOOP_CONTROL)
mode 3 5            # Set mode: control=3 (position), input=5 (trap_traj)
clear_err           # Clear errors
estop               # Emergency stop
get_pos             # Query encoder position (RTR)
get_iq              # Query current (RTR)
get_voltage         # Query bus voltage (RTR)
help                # Show command list
```

---

## CAN ID Mapping (node_id=0)

| CAN ID | Command Name | Direction | Type |
|--------|-------------|-----------|------|
| 0x001  | Heartbeat (cyclic) | OUT | Cyclic |
| 0x002  | E-Stop | IN | Write |
| 0x003  | Get Motor Error | IN | RTR → Response |
| 0x004  | Get Encoder Error | IN | RTR → Response |
| 0x005  | Get Sensorless Error | IN | RTR → Response |
| 0x007  | Set Axis State | IN | Write |
| 0x00B  | Set Controller Mode | IN | Write |
| 0x00C  | Set Input Position | IN | Write |
| 0x00D  | Set Input Velocity | IN | Write |
| 0x00E  | Set Input Torque | IN | Write |
| 0x00F  | Set Velocity Limit | IN | Write |
| 0x010  | Start Anticogging | IN | Write |
| 0x011  | Set Traj Vel Limit | IN | Write |
| 0x012  | Set Traj Accel Limits | IN | Write |
| 0x013  | Set Traj Inertia | IN | Write |
| 0x014  | Get Iq | IN | RTR → Response |
| 0x015  | Get Sensorless Estimates | IN | RTR → Response |
| 0x017  | Get Vbus Voltage | IN | RTR → Response |
| 0x018  | Clear Errors | IN | Write |
| 0x01D  | Reset ODrive | IN | Write |

---

## CAN Message Encoding Reference

All examples assume **node_id=0**, little-endian encoding.

### Control Sequence (Typical Usage)

1. **Set velocity limit** (optional):
   ```
   CAN ID: 0x00F
   Data: [vel_limit as float32]
   Example: Set to 5 rot/s
   Bytes: 0x00 0x00 0xA0 0x40 (little-endian IEEE 754 for 5.0)
   ```

2. **Set controller mode** (required first):
   ```
   CAN ID: 0x00B
   Data: [control_mode:int32] [input_mode:int32]
   Example: Position control (3) + trapezoid trajectory (5)
   Bytes: 0x03 0x00 0x00 0x00 0x05 0x00 0x00 0x00
   ```

3. **Set axis state to CLOSED_LOOP (6)**:
   ```
   CAN ID: 0x007
   Data: [state:int16]
   Example: State 6
   Bytes: 0x06 0x00
   ```

4. **Send position command** (repeating):
   ```
   CAN ID: 0x00C
   Data: [pos:float32] [vel_ff:int16@0.001] [torque_ff:int16@0.001]
   Example: pos=2.5 rot, vel_ff=0, torque_ff=0
   Bytes: 0x00 0x00 0x20 0x40 0x00 0x00 0x00 0x00
   ```

---

## Data Type Encoding (Little-Endian)

### float32 (IEEE 754)
```c
#include <cstring>
float value = 5.0f;
uint8_t buf[4];
memcpy(buf, &value, 4);  // LSB first (little-endian)
// buf[0] = 0x00, buf[1] = 0x00, buf[2] = 0xA0, buf[3] = 0x40
```

### int16 with scaling
```c
int16_t vel_ff = (int16_t)(velocity / 0.001);  // Scale down
uint8_t buf[2];
buf[0] = vel_ff & 0xFF;         // LSB
buf[1] = (vel_ff >> 8) & 0xFF;  // MSB
```

### uint32
```c
uint32_t value = 0x12345678;
uint8_t buf[4];
buf[0] = value & 0xFF;
buf[1] = (value >> 8) & 0xFF;
buf[2] = (value >> 16) & 0xFF;
buf[3] = (value >> 24) & 0xFF;
```

---

## Response Messages (RTR Queries)

### Query Pattern
1. Send RTR frame: `CAN_ID | RTR_FLAG` (data length = 0)
2. ODrive responds with data frame containing results

### Response Parsing Examples

**Get_Encoder_Estimates (0x009)** → Response with:
- Bytes 0-3: position as float32
- Bytes 4-7: velocity as float32

```c
float pos, vel;
memcpy(&pos, &rx_data[0], 4);  // Little-endian
memcpy(&vel, &rx_data[4], 4);
printf("Position: %f, Velocity: %f\n", pos, vel);
```

**Get_Iq (0x014)** → Response with:
- Bytes 0-3: Iq setpoint as float32 (A)
- Bytes 4-7: Iq measured as float32 (A)

**Get_Vbus_Voltage (0x017)** → Response with:
- Bytes 0-3: voltage as float32 (V)
- Bytes 4-7: padding (zeros)

---

## Axis States Reference
```
0 = UNDEFINED / IDLE
1 = UNDEFINED
2 = STARTUP_SEQUENCE
3 = CALIBRATE_USING_HOMOGRAPHY
4 = CALIBRATE_ANTI_COGGING
5 = reserved
6 = CLOSED_LOOP_CONTROL ← Common target
7 = LOCKIN_SPIN
8 = ENCODER_INDEX_SEARCH
9 = ENCODER_DIR_FIND
```

## Controller Modes
```
0 = VOLTAGE_CONTROL
1 = TORQUE_CONTROL
2 = VELOCITY_CONTROL
3 = POSITION_CONTROL ← For your arm
```

## Input Modes
```
0 = INACTIVE
1 = PASSTHROUGH (direct setpoint)
2 = VEL_RAMP
3 = POS_FILTER
4 = MIX_CHANNELS
5 = TRAP_TRAJ (trapezoid trajectory) ← Good for arm motion
6 = TORQUE_RAMP
```

---

## Serial-to-CAN Translator Implementation Checklist

### Phase 1: Basic Structure
- [ ] ESP-IDF project with MCP2515 CAN driver configured
- [ ] Serial input parser (readline from USB CDC)
- [ ] CAN message encoder/decoder utilities
- [ ] Test harness for message format validation

### Phase 2: Command Handlers
- [ ] Position command (0x00C) - SET_INPUT_POS
- [ ] Velocity command (0x00D) - SET_INPUT_VEL
- [ ] Controller mode setup (0x00B) - SET_CONTROLLER_MODES
- [ ] Axis state control (0x007) - SET_AXIS_REQUESTED_STATE
- [ ] E-stop (0x002) - ESTOP
- [ ] Clear errors (0x018) - CLEAR_ERRORS

### Phase 3: Query Handlers (RTR)
- [ ] Get encoder position (0x009) - GET_ENCODER_ESTIMATES
- [ ] Get current (0x014) - GET_IQ
- [ ] Get voltage (0x017) - GET_VBUS_VOLTAGE
- [ ] Get motor error (0x003) - GET_MOTOR_ERROR
- [ ] Get encoder error (0x004) - GET_ENCODER_ERROR

### Phase 4: Advanced Features
- [ ] Automated startup sequence (mode → state)
- [ ] Watchdog feed on send
- [ ] Response timeout handler
- [ ] Status display command (print heartbeat data)

---

## Testing Sequence

1. **Hardware check**:
   ```
   esp32# can_status
   esp32# can_init 250000  # 250 kbps
   ```

2. **ODrive setup** (via USB):
   ```
   odrv0.axis0.config.can_node_id = 0
   odrv0.save_configuration()
   ```

3. **Heartbeat test**:
   ```
   esp32# can_listen  # Should see 0x001 messages every 100ms
   ```

4. **Basic position command**:
   ```
   esp32# mode 3 5     # Position + Trap trajectory
   esp32# state 6      # Enter closed-loop
   esp32# pos 1.0 0 0  # Move to 1 rotation
   esp32# get_pos      # Query current position
   ```

5. **Diagnostic queries**:
   ```
   esp32# get_voltage  # Check bus voltage
   esp32# get_iq       # Check current draw
   ```

---

## Known Limitations (v0.5.1)

⚠️ **Output Encoder NOT Readable**: `Get_Encoder_Estimates` only returns motor encoder
- Your joint output encoder data is NOT available over CAN
- You'll need to read absolute encoder separately (not via ODrive CAN)

⚠️ **No Version Query**: No `Get_Version` command
- Hard-code firmware detection for v0.5.1

⚠️ **RTR-Only Feedback**: Encoder/current queries require RTR request
- Not automatic cyclic push like modern CAN drives
- Adds latency if you query every control cycle (careful with timing!)

---

## Serial Command Examples

```
# Initialization sequence
mode 3 5            # Position control + Trap traj
state 6             # Enter closed-loop
vel_limit 10        # 10 rot/sec max velocity

# Motion commands (repeatable)
pos 0.5 0 0         # Go to 0.5 rotations
pos 1.0 0 0         # Go to 1.0 rotations
pos 0 0 0           # Return to 0

# Diagnostics
get_pos             # Current motor position & velocity
get_iq              # Current draw (setpoint & measured)
get_voltage         # Bus voltage

# Emergency
estop               # Full stop + error flag
clear_err           # Clear error state
state 1             # Back to idle
```

---

## Recommended CAN Signal Helper Functions

```c
// Pack float32 into 4 bytes, little-endian position
void pack_float32(uint8_t* buf, int offset, float value) {
    uint32_t bits;
    memcpy(&bits, &value, 4);
    buf[offset+0] = bits & 0xFF;
    buf[offset+1] = (bits >> 8) & 0xFF;
    buf[offset+2] = (bits >> 16) & 0xFF;
    buf[offset+3] = (bits >> 24) & 0xFF;
}

// Unpack float32 from 4 bytes, little-endian position
float unpack_float32(const uint8_t* buf, int offset) {
    uint32_t bits = buf[offset+0] | 
                   (buf[offset+1] << 8) | 
                   (buf[offset+2] << 16) | 
                   (buf[offset+3] << 24);
    float value;
    memcpy(&value, &bits, 4);
    return value;
}

// Pack int16 into 2 bytes, little-endian
void pack_int16(uint8_t* buf, int offset, int16_t value) {
    buf[offset+0] = value & 0xFF;
    buf[offset+1] = (value >> 8) & 0xFF;
}

// Unpack int16 from 2 bytes, little-endian
int16_t unpack_int16(const uint8_t* buf, int offset) {
    return buf[offset+0] | (buf[offset+1] << 8);
}
```

---

## Troubleshooting

| Issue | Cause | Solution |
|-------|-------|----------|
| No CAN messages received | Wrong node ID | Verify `can_node_id = 0` via odrivetool |
| Heartbeat not appearing | Wrong baud rate | Check ODrive & ESP32 both set to 250k |
| Commands ignored | Not in CLOSED_LOOP state | Send `state 6` first |
| RTR queries timeout | Watchdog triggered | Check ODrive heartbeat LED; may need restart |
| Invalid data received | Endianness error | Verify little-endian encoding (check bytes) |
| Mode set fails | Controller error active | Send `clear_err` first |

