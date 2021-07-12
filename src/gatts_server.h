#ifndef MAIN_GATTS_SERVER_H_
#define MAIN_GATTS_SERVER_H_

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#define GATTS_DISPLAY_SERVICE_UUID   0x00FF
#define GATTS_HANDLE_COUNT     13

#define DEVICE_NAME            "BIKE_NAVIGATOR"
#define MANUFACTURER_DATA_LEN  17

#define CHAR_VAL_LEN_MAX 0x40

#define PREPARE_BUF_MAX_SIZE 1024

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#define PROFILE_A_APP_ID 0

#define MAX_CHAR_DATA_LEN 4
#define CHAR_COUNT 4

#define GATTS_DIR_CHAR_UUID 0xFF01
#define DIR_CHAR_INDEX 0
#define DIR_CHAR_DATA_LEN 1

#define GATTS_METERS_CHAR_UUID 0xFF02
#define METERS_CHAR_INDEX 1
#define METERS_CHAR_DATA_LEN 4

#define GATTS_SPEED_CHAR_UUID 0xFF03
#define SPEED_CHAR_INDEX 2
#define SPEED_CHAR_DATA_LEN 1

#define GATTS_MODE_CHAR_UUID 0xFF04
#define MODE_CHAR_INDEX 3
#define MODE_CHAR_DATA_LEN 1


typedef enum {
    NO_DIR,
    TURN_SHARP_LEFT,
    UTURN_RIGHT,
    TURN_SLIGHT_RIGHT,
    MERGE,
    ROUNDABOUT_LEFT,
    ROUNDABOUT_RIGHT,
    UTURN_LEFT,
    TURN_SLIGHT_LEFT,
    TURN_LEFT,
    RAMP_RIGHT,
    TURN_RIGHT,
    FORK_RIGHT,
    STRAIGHT,
    FORK_LEFT,
    FERRY_TRAIN,
    TURN_SHARP_RIGHT,
    RAMP_LEFT,
    FERRY
} direction; 


typedef enum {
    NOTHING,
    NAVIGATION,
    SPEEDOMETER
} mode; 


struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;

    uint16_t char_handle[CHAR_COUNT];
    esp_bt_uuid_t char_uuid[CHAR_COUNT];
    esp_attr_value_t char_attr[CHAR_COUNT];

    esp_bt_uuid_t descr_uuid[CHAR_COUNT];
};
struct dir_data {
    direction dir;
    uint32_t meters;
    uint8_t speed;
    mode mode;
};


struct gatts_profile_inst gl_profile;

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void setup_ble();
bool get_dir_status(struct dir_data* out);
bool is_bt_connected();

#endif
