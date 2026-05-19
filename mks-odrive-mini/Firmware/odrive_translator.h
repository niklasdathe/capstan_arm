// odrive_translator.h
// Header for ODrive CAN message translator

#ifndef ODRIVE_TRANSLATOR_H
#define ODRIVE_TRANSLATOR_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/can.h"

#define ODRIVE_NODE_ID 0
#define ODRIVE_CAN_BAUDRATE 250000

// Message IDs for ODrive v0.5.1 CANSimple
typedef enum {
    MSG_CO_NMT_CTRL = 0x000,
    MSG_ODRIVE_HEARTBEAT = 0x001,
    MSG_ODRIVE_ESTOP = 0x002,
    MSG_GET_MOTOR_ERROR = 0x003,
    MSG_GET_ENCODER_ERROR = 0x004,
    MSG_GET_SENSORLESS_ERROR = 0x005,
    MSG_SET_AXIS_NODE_ID = 0x006,
    MSG_SET_AXIS_REQUESTED_STATE = 0x007,
    MSG_SET_AXIS_STARTUP_CONFIG = 0x008,
    MSG_GET_ENCODER_ESTIMATES = 0x009,
    MSG_GET_ENCODER_COUNT = 0x00A,
    MSG_SET_CONTROLLER_MODES = 0x00B,
    MSG_SET_INPUT_POS = 0x00C,
    MSG_SET_INPUT_VEL = 0x00D,
    MSG_SET_INPUT_TORQUE = 0x00E,
    MSG_SET_VEL_LIMIT = 0x00F,
    MSG_START_ANTICOGGING = 0x010,
    MSG_SET_TRAJ_VEL_LIMIT = 0x011,
    MSG_SET_TRAJ_ACCEL_LIMITS = 0x012,
    MSG_SET_TRAJ_INERTIA = 0x013,
    MSG_GET_IQ = 0x014,
    MSG_GET_SENSORLESS_ESTIMATES = 0x015,
    MSG_GET_VBUS_VOLTAGE = 0x017,
    MSG_CLEAR_ERRORS = 0x018,
    MSG_RESET_ODRIVE = 0x01D,
} odrive_msg_id_t;

// Axis states
typedef enum {
    AXIS_STATE_UNDEFINED = 0,
    AXIS_STATE_IDLE = 1,
    AXIS_STATE_STARTUP_SEQUENCE = 2,
    AXIS_STATE_CALIBRATE_USING_HOMOGRAPHY = 3,
    AXIS_STATE_CALIBRATE_ANTI_COGGING = 4,
    AXIS_STATE_RESERVED_5 = 5,
    AXIS_STATE_CLOSED_LOOP_CONTROL = 6,
    AXIS_STATE_LOCKIN_SPIN = 7,
    AXIS_STATE_ENCODER_INDEX_SEARCH = 8,
    AXIS_STATE_ENCODER_DIR_FIND = 9,
} axis_state_t;

// Control modes
typedef enum {
    CONTROL_MODE_VOLTAGE_CONTROL = 0,
    CONTROL_MODE_TORQUE_CONTROL = 1,
    CONTROL_MODE_VELOCITY_CONTROL = 2,
    CONTROL_MODE_POSITION_CONTROL = 3,
} control_mode_t;

// Input modes
typedef enum {
    INPUT_MODE_INACTIVE = 0,
    INPUT_MODE_PASSTHROUGH = 1,
    INPUT_MODE_VEL_RAMP = 2,
    INPUT_MODE_POS_FILTER = 3,
    INPUT_MODE_MIX_CHANNELS = 4,
    INPUT_MODE_TRAP_TRAJ = 5,
    INPUT_MODE_TORQUE_RAMP = 6,
} input_mode_t;

// CAN message utilities
static inline uint32_t odrive_calc_can_id(uint8_t node_id, uint8_t cmd_id) {
    return (node_id << 5) | (cmd_id & 0x1F);
}

static inline void pack_float32_le(uint8_t* buf, int offset, float value) {
    uint32_t bits;
    memcpy(&bits, &value, 4);
    buf[offset] = bits & 0xFF;
    buf[offset+1] = (bits >> 8) & 0xFF;
    buf[offset+2] = (bits >> 16) & 0xFF;
    buf[offset+3] = (bits >> 24) & 0xFF;
}

static inline float unpack_float32_le(const uint8_t* buf, int offset) {
    uint32_t bits = buf[offset] | 
                   (buf[offset+1] << 8) | 
                   (buf[offset+2] << 16) | 
                   (buf[offset+3] << 24);
    float value;
    memcpy(&value, &bits, 4);
    return value;
}

static inline void pack_int16_le(uint8_t* buf, int offset, int16_t value) {
    buf[offset] = value & 0xFF;
    buf[offset+1] = (value >> 8) & 0xFF;
}

static inline int16_t unpack_int16_le(const uint8_t* buf, int offset) {
    return buf[offset] | (buf[offset+1] << 8);
}

static inline void pack_int32_le(uint8_t* buf, int offset, int32_t value) {
    buf[offset] = value & 0xFF;
    buf[offset+1] = (value >> 8) & 0xFF;
    buf[offset+2] = (value >> 16) & 0xFF;
    buf[offset+3] = (value >> 24) & 0xFF;
}

static inline int32_t unpack_int32_le(const uint8_t* buf, int offset) {
    return buf[offset] | 
          (buf[offset+1] << 8) | 
          (buf[offset+2] << 16) | 
          (buf[offset+3] << 24);
}

// Public API
void odrive_translator_init(void);
void odrive_send_set_input_pos(float position, float vel_feedforward, float torque_feedforward);
void odrive_send_set_input_vel(float velocity, float torque_feedforward);
void odrive_send_set_input_torque(float torque);
void odrive_send_set_controller_modes(control_mode_t control_mode, input_mode_t input_mode);
void odrive_send_set_axis_state(axis_state_t state);
void odrive_send_set_vel_limit(float vel_limit);
void odrive_send_estop(void);
void odrive_send_clear_errors(void);
void odrive_send_query_encoder_estimates(void);
void odrive_send_query_iq(void);
void odrive_send_query_vbus_voltage(void);

#endif // ODRIVE_TRANSLATOR_H
