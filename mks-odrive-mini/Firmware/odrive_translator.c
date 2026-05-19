// odrive_translator.c
// Implementation of ODrive CAN message translator for ESP32-S3

#include "odrive_translator.h"
#include "driver/can.h"
#include "esp_log.h"
#include <string.h>

static const char* TAG = "ODrive_Translator";

// CAN message struct for frame transmission
typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    bool rtr;
    bool is_extended;
} odrive_can_frame_t;

// Initialize CAN interface (should be called once during setup)
void odrive_translator_init(void) {
    // CAN configuration is typically done in your main.c
    ESP_LOGI(TAG, "ODrive translator initialized");
}

// Helper: Send CAN frame
static void odrive_send_frame(uint8_t cmd_id, const uint8_t* data, uint8_t dlc, bool rtr) {
    can_message_t msg;
    msg.identifier = odrive_calc_can_id(ODRIVE_NODE_ID, cmd_id);
    msg.flags = rtr ? CAN_MSG_FLAG_RTR : 0;
    msg.data_length_code = dlc;
    
    if (data && dlc > 0) {
        memcpy(msg.data, data, dlc);
    } else {
        memset(msg.data, 0, 8);
    }
    
    esp_err_t result = can_transmit(&msg, pdMS_TO_TICKS(10));
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "CAN transmit failed for cmd_id 0x%02X: %s", cmd_id, esp_err_to_name(result));
    }
}

// 0x00C - Set Input Position
void odrive_send_set_input_pos(float position, float vel_feedforward, float torque_feedforward) {
    uint8_t data[8];
    pack_float32_le(data, 0, position);
    
    // Scale velocity feedforward by 0.001
    int16_t vel_ff_scaled = (int16_t)(vel_feedforward / 0.001f);
    pack_int16_le(data, 4, vel_ff_scaled);
    
    // Scale torque feedforward by 0.001
    int16_t torque_ff_scaled = (int16_t)(torque_feedforward / 0.001f);
    pack_int16_le(data, 6, torque_ff_scaled);
    
    odrive_send_frame(MSG_SET_INPUT_POS, data, 8, false);
    ESP_LOGI(TAG, "Set position: %.2f rot, vel_ff: %.3f, torque_ff: %.3f", 
             position, vel_feedforward, torque_feedforward);
}

// 0x00D - Set Input Velocity
void odrive_send_set_input_vel(float velocity, float torque_feedforward) {
    uint8_t data[8];
    pack_float32_le(data, 0, velocity);
    pack_float32_le(data, 4, torque_feedforward);
    
    odrive_send_frame(MSG_SET_INPUT_VEL, data, 8, false);
    ESP_LOGI(TAG, "Set velocity: %.2f rot/s, torque_ff: %.3f", velocity, torque_feedforward);
}

// 0x00E - Set Input Torque
void odrive_send_set_input_torque(float torque) {
    uint8_t data[4];
    pack_float32_le(data, 0, torque);
    
    odrive_send_frame(MSG_SET_INPUT_TORQUE, data, 4, false);
    ESP_LOGI(TAG, "Set torque: %.3f Nm", torque);
}

// 0x00B - Set Controller Modes
void odrive_send_set_controller_modes(control_mode_t control_mode, input_mode_t input_mode) {
    uint8_t data[8];
    pack_int32_le(data, 0, (int32_t)control_mode);
    pack_int32_le(data, 4, (int32_t)input_mode);
    
    odrive_send_frame(MSG_SET_CONTROLLER_MODES, data, 8, false);
    ESP_LOGI(TAG, "Set modes: control=%d, input=%d", control_mode, input_mode);
}

// 0x007 - Set Axis Requested State
void odrive_send_set_axis_state(axis_state_t state) {
    uint8_t data[2];
    pack_int16_le(data, 0, (int16_t)state);
    
    odrive_send_frame(MSG_SET_AXIS_REQUESTED_STATE, data, 2, false);
    ESP_LOGI(TAG, "Set axis state: %d", state);
}

// 0x00F - Set Velocity Limit
void odrive_send_set_vel_limit(float vel_limit) {
    uint8_t data[4];
    pack_float32_le(data, 0, vel_limit);
    
    odrive_send_frame(MSG_SET_VEL_LIMIT, data, 4, false);
    ESP_LOGI(TAG, "Set velocity limit: %.2f rot/s", vel_limit);
}

// 0x002 - E-Stop
void odrive_send_estop(void) {
    odrive_send_frame(MSG_ODRIVE_ESTOP, NULL, 0, false);
    ESP_LOGW(TAG, "E-STOP sent!");
}

// 0x018 - Clear Errors
void odrive_send_clear_errors(void) {
    odrive_send_frame(MSG_CLEAR_ERRORS, NULL, 0, false);
    ESP_LOGI(TAG, "Clear errors sent");
}

// 0x009 - Get Encoder Estimates (RTR)
void odrive_send_query_encoder_estimates(void) {
    odrive_send_frame(MSG_GET_ENCODER_ESTIMATES, NULL, 0, true);
    ESP_LOGI(TAG, "Query encoder estimates sent");
}

// 0x014 - Get Iq (RTR)
void odrive_send_query_iq(void) {
    odrive_send_frame(MSG_GET_IQ, NULL, 0, true);
    ESP_LOGI(TAG, "Query Iq sent");
}

// 0x017 - Get Vbus Voltage (RTR)
void odrive_send_query_vbus_voltage(void) {
    odrive_send_frame(MSG_GET_VBUS_VOLTAGE, NULL, 0, true);
    ESP_LOGI(TAG, "Query vbus voltage sent");
}

// Helper: Parse heartbeat message (0x001)
void odrive_parse_heartbeat(const can_message_t* msg) {
    if (msg->identifier != odrive_calc_can_id(ODRIVE_NODE_ID, MSG_ODRIVE_HEARTBEAT)) {
        return;
    }
    
    uint32_t error = unpack_int32_le(msg->data, 0);
    uint32_t state = unpack_int32_le(msg->data, 4);
    
    ESP_LOGI(TAG, "Heartbeat - Errors: 0x%08X, State: %d", error, state);
}

// Helper: Parse query response for encoder estimates (0x009)
void odrive_parse_encoder_estimates(const can_message_t* msg) {
    if (msg->identifier != odrive_calc_can_id(ODRIVE_NODE_ID, MSG_GET_ENCODER_ESTIMATES)) {
        return;
    }
    
    float position = unpack_float32_le(msg->data, 0);
    float velocity = unpack_float32_le(msg->data, 4);
    
    ESP_LOGI(TAG, "Encoder: pos=%.4f rot, vel=%.4f rot/s", position, velocity);
}

// Helper: Parse query response for Iq (0x014)
void odrive_parse_iq(const can_message_t* msg) {
    if (msg->identifier != odrive_calc_can_id(ODRIVE_NODE_ID, MSG_GET_IQ)) {
        return;
    }
    
    float iq_setpoint = unpack_float32_le(msg->data, 0);
    float iq_measured = unpack_float32_le(msg->data, 4);
    
    ESP_LOGI(TAG, "Current: Iq_setpoint=%.3f A, Iq_measured=%.3f A", iq_setpoint, iq_measured);
}

// Helper: Parse query response for Vbus voltage (0x017)
void odrive_parse_vbus_voltage(const can_message_t* msg) {
    if (msg->identifier != odrive_calc_can_id(ODRIVE_NODE_ID, MSG_GET_VBUS_VOLTAGE)) {
        return;
    }
    
    float vbus = unpack_float32_le(msg->data, 0);
    
    ESP_LOGI(TAG, "Bus Voltage: %.2f V", vbus);
}
