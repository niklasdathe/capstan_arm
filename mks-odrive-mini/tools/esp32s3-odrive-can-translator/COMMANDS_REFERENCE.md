# ODrive CAN Translator - Serial Commands Quick Reference

## Connection
- **Baud Rate:** 115200
- **Serial Port:** /dev/ttyACM0 (Linux/Mac) or COM3+ (Windows)

---

## Essential Commands

### Get Help
```
help                 Print all available commands
```

### Motion Control
```
pos <pos> <vf> <tf>  Set target position (rotations, vel_ff, torque_ff)
vel <vel> <tf>       Set target velocity (rot/s, torque_ff)
torque <value>       Set direct torque (Nm)
```

### Configuration
```
mode <cm> <im>       Set control_mode input_mode
state <s>            Set axis state (6=closed_loop)
vel_limit <v>        Set velocity limit (rot/s)
```

### Status & Diagnostics
```
get_pos              Query encoder position and velocity
get_iq               Query motor current
get_voltage          Query bus voltage
clear_err            Clear all error flags
estop                Emergency stop (immediate)
```

---

## Common Mode Values

| Mode | Value | Description |
|------|-------|-------------|
| **Control Mode** | 3 | Position Control (most common) |
| **Input Mode** | 5 | Trapezoid Trajectory |
| **Axis State** | 6 | Closed-loop Control |

---

## Typical Usage Session

```
$ help
$ clear_err
$ mode 3 5
$ state 6
$ pos 1.0 0 0
$ get_pos
```

---

## Control Modes (Full List)

**control_mode:**
| Value | Name |
|-------|------|
| 0 | Voltage Control |
| 1 | Torque Control |
| 2 | Velocity Control |
| 3 | **Position Control** ⭐ |

**input_mode:**
| Value | Name |
|-------|------|
| 0 | Inactive |
| 1 | Passthrough |
| 2 | Velocity Ramp |
| 3 | Position Filter |
| 4 | Mix Channels |
| 5 | **Trapezoid Trajectory** ⭐ |
| 6 | Torque Ramp |

**axis_state:**
| Value | Name |
|-------|------|
| 1 | Idle |
| 2 | Startup Sequence |
| 4 | Calibrate Anticogging |
| 6 | **Closed-loop Control** ⭐ |
| 8 | Encoder Index Search |

---

## Command Examples

### Simple Position Move
```
pos 0
pos 0.5
pos 1.0
pos -0.5
```

### Velocity Control
```
mode 2 1             (velocity mode + passthrough)
vel 1.0 0
vel 5.0 0
vel 0                (stop)
```

### Emergency Stop
```
estop                (immediately stops motor)
clear_err
```

### Diagnostics
```
get_voltage          (should be ~24V typical)
get_pos              (current position and speed)
get_iq               (motor current)
```

---

## Hardware Info

- **Board:** XIAO ESP32-S3
- **CAN TX:** GPIO 21
- **CAN RX:** GPIO 20
- **CAN Speed:** 250 kbps
- **ODrive Node ID:** 0 (must match ODrive config)

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| No response to commands | Check serial port, baud rate (115200) |
| "Motor not moving" | Run `state 6` to enter closed-loop mode |
| Errors in logs | Run `clear_err` to clear flags |
| CAN not responding | Check GPIO 21/20 connections, verify CAN terminator |

---

## Pro Tips

1. **Always `clear_err` first** after connecting
2. **Set mode before state** (mode → state → motion)
3. **Use `get_pos` for feedback** before next command
4. **`estop` is always available** for emergency stop
5. **Serial errors during idle are normal**

