// main.c - Example ESP32-S3 serial-to-CAN translator
// This example shows how to parse serial commands and send CAN messages

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/can.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "odrive_translator.h"

static const char* TAG = "ESP32-S3_Translator";

// CAN configuration
#define TX_GPIO_NUM 41      // GPIO41 for CAN TX (adjust for your hat)
#define RX_GPIO_NUM 40      // GPIO40 for CAN RX (adjust for your hat)
#define CAN_BAUDRATE 250000 // 250 kbps

// UART for serial communication
#define UART_NUM UART_NUM_0
#define BUF_SIZE 1024

// Queue for CAN RX messages
static QueueHandle_t rx_queue;

// CAN RX message handler task
static void can_rx_task(void* arg) {
    can_message_t rx_msg;
    
    while (1) {
        if (xQueueReceive(rx_queue, &rx_msg, portMAX_DELAY)) {
            uint32_t msg_id = rx_msg.identifier;
            uint8_t cmd_id = msg_id & 0x1F;
            
            // Parse known response messages
            switch (cmd_id) {
                case MSG_ODRIVE_HEARTBEAT:
                    odrive_parse_heartbeat(&rx_msg);
                    break;
                case MSG_GET_ENCODER_ESTIMATES:
                    odrive_parse_encoder_estimates(&rx_msg);
                    break;
                case MSG_GET_IQ:
                    odrive_parse_iq(&rx_msg);
                    break;
                case MSG_GET_VBUS_VOLTAGE:
                    odrive_parse_vbus_voltage(&rx_msg);
                    break;
                case MSG_GET_MOTOR_ERROR:
                    {
                        uint32_t error = unpack_int32_le(rx_msg.data, 0);
                        ESP_LOGW(TAG, "Motor Error: 0x%08X", error);
                    }
                    break;
                default:
                    ESP_LOGD(TAG, "Received msg_id: 0x%03X", msg_id);
                    break;
            }
        }
    }
}

// CAN initialization
static void can_init(void) {
    // CAN driver configuration
    can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM,
                                                                CAN_MODE_NORMAL);
    can_timing_config_t t_config = CAN_TIMING_CONFIG_250KBITS();
    can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
    
    // Install CAN driver
    if (can_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install CAN driver");
        return;
    }
    
    // Start CAN driver
    if (can_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start CAN driver");
        return;
    }
    
    ESP_LOGI(TAG, "CAN driver initialized at %d kbps", CAN_BAUDRATE / 1000);
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
    rx_queue = xQueueCreate(10, sizeof(can_message_t));
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

/* ===== CMakeLists.txt for ESP-IDF project =====

idf_component_register(SRCS 
                        "main.c" 
                        "odrive_translator.c"
                        INCLUDE_DIRS "."
                        REQUIRES driver freertos esp_common)

===== idf_component.yml =====

version: "1.0.0"
description: "ODrive CAN translator for ESP32-S3"
dependencies:
  esp_system:
    version: "*"
  esp_driver_can:
    version: "*"
  esp_driver_uart:
    version: "*"

===== Optional: sdkconfig =====

CONFIG_CAN_DRIVER_ENABLED=y
CONFIG_CAN_RX_MSG_QUEUE_SIZE=10
CONFIG_UART_ISR_IN_IRAM=y
CONFIG_FREERTOS_HZ=1000

*/
