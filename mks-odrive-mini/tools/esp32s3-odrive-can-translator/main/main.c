// main.c - ESP32-S3 serial-to-CAN translator for ODrive
// This application translates serial commands into CAN messages for ODrive v0.5.1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_twai.h"
#include "esp_twai_types.h"
#include "esp_twai_onchip.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "odrive_translator.h"

static const char* TAG = "ESP32-S3_Translator";

// ===== XIAO ESP32-S3 Pinout Configuration =====
// For XIAO ESP32-S3 with on-chip TWAI (CAN):
//   TX: GPIO 21
//   RX: GPIO 20
//
// For external MCP2515 via SPI (alternative):
//   CS:   GPIO 44
//   MOSI: GPIO 9
//   MISO: GPIO 8
//   SCK:  GPIO 7
//
// Adjust the pins below to match your hardware setup:
#define TX_GPIO_NUM 21      // CAN TX pin (adjust for your board/MCP2515 setup)
#define RX_GPIO_NUM 20      // CAN RX pin (adjust for your board/MCP2515 setup)
#define CAN_BAUDRATE 250000 // 250 kbps

// UART for serial communication
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

// Queue for CAN RX messages
static QueueHandle_t rx_queue;
twai_node_handle_t twai_node = NULL;  // Global (non-static) so translator.c can access it

// CAN RX callback - placeholder (RX data not directly available in event)
// For now, we'll focus on TX functionality
// static bool twai_rx_callback(twai_node_handle_t node, const twai_rx_done_event_data_t *edata, void *user_ctx) {
//     // Event-driven RX is more complex in the new API
//     // For basic functionality, we send commands and skip response parsing for now
//     return false;
// }

// CAN RX message handler task - simplified version
static void can_rx_task(void* arg) {
    ESP_LOGI(TAG, "CAN RX task started (note: RX responses not actively monitored yet)");
    
    while (1) {
        // Simple placeholder - just monitor the bus periodically
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// CAN initialization
static void can_init(void) {
    ESP_LOGI(TAG, "Initializing TWAI (CAN) driver...");

    // Configure TWAI node with simplified settings
    twai_onchip_node_config_t twai_config = {
        .io_cfg = {
            .tx = TX_GPIO_NUM,
            .rx = RX_GPIO_NUM,
            .quanta_clk_out = -1,
            .bus_off_indicator = -1,
        },
        .clk_src = TWAI_CLK_SRC_DEFAULT,
        .bit_timing = {
            .bitrate = 250000,  // 250 kbps
        },
        .data_timing = {},
        .timestamp_resolution_hz = 0,
        .fail_retry_cnt = -1,
        .tx_queue_depth = 32,
        .intr_priority = 1,
        .flags = {
            .enable_loopback = 0,
            .enable_listen_only = 0,
            .no_receive_rtr = 0,
        },
    };

    // Create TWAI node
    if (twai_new_node_onchip(&twai_config, &twai_node) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create TWAI node");
        return;
    }

    // Enable the node
    if (twai_node_enable(twai_node) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable TWAI node");
        return;
    }

    ESP_LOGI(TAG, "TWAI driver initialized at 250 kbps");
}

// Serial command parser
static void parse_serial_command(char* cmd) {
    char cmd_copy[BUF_SIZE];
    strncpy(cmd_copy, cmd, BUF_SIZE - 1);
    cmd_copy[BUF_SIZE - 1] = '\0';
    
    // Tokenize command
    char* token = strtok(cmd_copy, " ");
    if (!token) return;
    
    // Parse commands
    if (strcmp(token, "help") == 0) {
        printf("\n=== ODrive CAN Command Reference ===\n");
        printf("pos <float> <float> <float>   - Set position, vel_ff, torque_ff\n");
        printf("vel <float> <float>           - Set velocity, torque_ff\n");
        printf("torque <float>                - Set torque\n");
        printf("mode <int> <int>              - Set control_mode input_mode\n");
        printf("state <int>                   - Set axis state (6=closed_loop)\n");
        printf("vel_limit <float>             - Set velocity limit\n");
        printf("get_pos                       - Query encoder position\n");
        printf("get_iq                        - Query current\n");
        printf("get_voltage                   - Query bus voltage\n");
        printf("estop                         - Emergency stop\n");
        printf("clear_err                     - Clear errors\n");
        printf("=====================================\n\n");
        
    } else if (strcmp(token, "pos") == 0) {
        float pos = atof(strtok(NULL, " ") ?: "0");
        float vel_ff = atof(strtok(NULL, " ") ?: "0");
        float torque_ff = atof(strtok(NULL, " ") ?: "0");
        odrive_send_set_input_pos(pos, vel_ff, torque_ff);
        
    } else if (strcmp(token, "vel") == 0) {
        float vel = atof(strtok(NULL, " ") ?: "0");
        float torque_ff = atof(strtok(NULL, " ") ?: "0");
        odrive_send_set_input_vel(vel, torque_ff);
        
    } else if (strcmp(token, "torque") == 0) {
        float torque = atof(strtok(NULL, " ") ?: "0");
        odrive_send_set_input_torque(torque);
        
    } else if (strcmp(token, "mode") == 0) {
        int ctrl_mode = atoi(strtok(NULL, " ") ?: "0");
        int input_mode = atoi(strtok(NULL, " ") ?: "0");
        odrive_send_set_controller_modes((control_mode_t)ctrl_mode, (input_mode_t)input_mode);
        
    } else if (strcmp(token, "state") == 0) {
        int state = atoi(strtok(NULL, " ") ?: "0");
        odrive_send_set_axis_state((axis_state_t)state);
        
    } else if (strcmp(token, "vel_limit") == 0) {
        float limit = atof(strtok(NULL, " ") ?: "5");
        odrive_send_set_vel_limit(limit);
        
    } else if (strcmp(token, "get_pos") == 0) {
        odrive_send_query_encoder_estimates();
        
    } else if (strcmp(token, "get_iq") == 0) {
        odrive_send_query_iq();
        
    } else if (strcmp(token, "get_voltage") == 0) {
        odrive_send_query_vbus_voltage();
        
    } else if (strcmp(token, "estop") == 0) {
        odrive_send_estop();
        
    } else if (strcmp(token, "clear_err") == 0) {
        odrive_send_clear_errors();
        
    } else {
        printf("Unknown command: %s (type 'help' for list)\n", token);
    }
}

// Serial input task
static void serial_input_task(void* arg) {
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';
            
            // Remove newline/carriage return
            char* str = (char*)data;
            str[strcspn(str, "\r\n")] = 0;
            
            if (strlen(str) > 0) {
                printf("$ %s\n", str);
                parse_serial_command(str);
            }
        }
    }
    
    free(data);
}

// Initialization sequence example
static void init_odrive_example(void) {
    printf("\n=== ODrive Initialization Sequence ===\n");
    printf("1. Clearing errors...\n");
    odrive_send_clear_errors();
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("2. Setting velocity limit to 10 rot/s...\n");
    odrive_send_set_vel_limit(10.0f);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("3. Setting control mode: Position (3) + Trap trajectory (5)...\n");
    odrive_send_set_controller_modes(CONTROL_MODE_POSITION_CONTROL, INPUT_MODE_TRAP_TRAJ);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("4. Entering closed-loop control (state 6)...\n");
    odrive_send_set_axis_state(AXIS_STATE_CLOSED_LOOP_CONTROL);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("=== Ready for commands ===\n");
    printf("Try: pos 1.0 0 0    (move to 1.0 rotation)\n");
    printf("Try: get_pos        (query current position)\n");
    printf("Try: help           (show all commands)\n\n");
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting ODrive CAN Translator on ESP32-S3");
    
    // Initialize CAN driver
    can_init();
    
    // Create RX queue and task
    rx_queue = xQueueCreate(10, sizeof(twai_message_t));
    xTaskCreate(can_rx_task, "CAN_RX", 2048, NULL, 5, NULL);
    
    // Start serial input task
    xTaskCreate(serial_input_task, "Serial_Input", 2048, NULL, 5, NULL);
    
    // Initialize translator
    odrive_translator_init();
    
    // Optional: Run initialization sequence
    vTaskDelay(pdMS_TO_TICKS(500));
    init_odrive_example();
    
    // Main task keeps running
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
