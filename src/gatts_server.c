#include "gatts_server.h"

#define tag "gatt"

bool bt_connected = false;

esp_gatt_char_prop_t common_char_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

uint8_t adv_config_done = 0;

uint8_t adv_service_uuid128[32] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006, 
    .max_interval = 0x0010, 
    .appearance = 0x00,
    .manufacturer_len = 0, 
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .appearance = 0x00,
    .manufacturer_len = 0, 
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(adv_service_uuid128),
    .p_service_uuid = adv_service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

esp_ble_adv_params_t adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


uint8_t dir_char_attr_value[DIR_CHAR_DATA_LEN] = {NO_DIR};
uint8_t meters_char_attr_value[METERS_CHAR_DATA_LEN] = {0,0,0,0};
uint8_t speed_char_attr_value[SPEED_CHAR_DATA_LEN] = {0};
uint8_t mode_char_attr_value[MODE_CHAR_DATA_LEN] = {0};
struct gatts_profile_inst gl_profile = {
    .gatts_cb = gatts_event_handler,
    .gatts_if = ESP_GATT_IF_NONE,
    .char_attr = {
        [DIR_CHAR_INDEX] = {
            .attr_max_len = CHAR_VAL_LEN_MAX,
            .attr_len     = DIR_CHAR_DATA_LEN,
            .attr_value   = dir_char_attr_value
        }, 
        [METERS_CHAR_INDEX] = {
            .attr_max_len = CHAR_VAL_LEN_MAX,
            .attr_len     = METERS_CHAR_DATA_LEN,
            .attr_value   = meters_char_attr_value
        }, 
        [SPEED_CHAR_INDEX] = {
            .attr_max_len = CHAR_VAL_LEN_MAX,
            .attr_len     = SPEED_CHAR_DATA_LEN,
            .attr_value   = speed_char_attr_value
        }, 
        [MODE_CHAR_INDEX] = {
            .attr_max_len = CHAR_VAL_LEN_MAX,
            .attr_len     = MODE_CHAR_DATA_LEN,
            .attr_value   = mode_char_attr_value
        },
    },
    .char_uuid = {
        [DIR_CHAR_INDEX] = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = GATTS_DIR_CHAR_UUID,
        },
        [METERS_CHAR_INDEX] = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = GATTS_METERS_CHAR_UUID,
        },
        [SPEED_CHAR_INDEX] = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = GATTS_SPEED_CHAR_UUID,
        },
        [MODE_CHAR_INDEX] = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = GATTS_MODE_CHAR_UUID,
        },
    },
    .descr_uuid = {
        [DIR_CHAR_INDEX] = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG
        },
        [METERS_CHAR_INDEX] = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG
        },
        [SPEED_CHAR_INDEX] = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG
        },
        [MODE_CHAR_INDEX] = {
            .len = ESP_UUID_LEN_16,
            .uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG
        },
    }
};



void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0){
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(tag, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(tag, "Advertising stop failed\n");
        } else {
            ESP_LOGI(tag, "Stop adv successfully\n");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
         ESP_LOGI(tag, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

esp_err_t add_characteristic(int char_index){
    ESP_LOGI(tag, "Adding characteristic with uuid=0x%x and index=%d",gl_profile.char_uuid[char_index].uuid.uuid16,char_index);
    return esp_ble_gatts_add_char(
        gl_profile.service_handle, 
        &gl_profile.char_uuid[char_index],
        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
        common_char_property,
        &gl_profile.char_attr[char_index], 
        NULL
    );
}

void set_char_attr_value(uint8_t char_index,esp_ble_gatts_cb_param_t *param){
    int char_data_len = gl_profile.char_attr[char_index].attr_len;
    if (param->write.len == char_data_len){
        ESP_LOGI(tag, "Setting new value");

        uint8_t new_char_value[MAX_CHAR_DATA_LEN] = {0};

        for(int i=0;i<char_data_len;i++){
            new_char_value[i] = param->write.value[i];
        }
        esp_ble_gatts_set_attr_value(gl_profile.char_handle[char_index],char_data_len,new_char_value);
        esp_ble_gatts_send_indicate(gl_profile.gatts_if, param->write.conn_id, gl_profile.char_handle[char_index],char_data_len, new_char_value, false);
    }
}

void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    case ESP_GATTS_REG_EVT:
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile.gatts_if = gatts_if;
        } else {
            ESP_LOGI(tag, "Reg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }

        ESP_LOGI(tag, "REGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        gl_profile.service_id.is_primary = true;
        gl_profile.service_id.id.inst_id = 0x00;
        gl_profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile.service_id.id.uuid.uuid.uuid16 = GATTS_DISPLAY_SERVICE_UUID;

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
        if (set_dev_name_ret){
            ESP_LOGE(tag, "set device name failed, error code = %x", set_dev_name_ret);
        }

        //config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret){
            ESP_LOGE(tag, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        //config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret){
            ESP_LOGE(tag, "config scan response data failed, error code = %x", ret);
        }
        adv_config_done |= scan_rsp_config_flag;

        ret = esp_ble_gatts_create_service(gatts_if, &gl_profile.service_id, GATTS_HANDLE_COUNT);
        if (ret){
            ESP_LOGE(tag, "create service failed, error code = %x", ret);
        }
        break;
    case ESP_GATTS_READ_EVT: {
        ESP_LOGI(tag, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));

        uint16_t length = 0;
        const uint8_t *prf_char;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->read.handle,  &length, &prf_char);
        if (get_attr_ret == ESP_FAIL){
            ESP_LOGE(tag, "Could not get attribute value (Handle %x)",param->read.handle);
        }

        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = length;
        for(int i=0;i<length;i++){
            rsp.attr_value.value[i] = prf_char[i];
        }
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
        ESP_LOGI(tag, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (param->write.is_prep){
            ESP_LOGW(tag,"Prep writes are not supported!!!");
            if (param->write.need_rsp){
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_WRITE_NOT_PERMIT, NULL);
            }
            return;
        }

        ESP_LOGI(tag, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
        esp_log_buffer_hex(tag, param->write.value, param->write.len);
        
        for (int i = 0; i < CHAR_COUNT; i++){
            if(gl_profile.char_handle[i] == param->write.handle){
                set_char_attr_value(i,param);
            }
        }

        if (param->write.need_rsp){
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        }
        break;
    }
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(tag, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(tag, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
        gl_profile.service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(gl_profile.service_handle);

        for (int i = 0; i < CHAR_COUNT; i++){
            esp_err_t add_char_ret = add_characteristic(i);
            if(add_char_ret){
                ESP_LOGE(tag, "add char failed, error code =%x",add_char_ret);
            }
        }
        break;
    case ESP_GATTS_ADD_CHAR_EVT: {
        uint16_t length = 0;
        const uint8_t *prf_char;

        ESP_LOGI(tag, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        
        if(param->add_char.status != ESP_GATT_OK){
            ESP_LOGE(tag, "Error %d while creating characteristic with uuid=%x",param->add_char.status,param->add_char.char_uuid.uuid.uuid16);
            return;
        }

        for(int i=0;i<CHAR_COUNT;i++){
            if(param->add_char.char_uuid.uuid.uuid16 != gl_profile.char_uuid[i].uuid.uuid16){
                continue;
            }

            gl_profile.char_handle[i] = param->add_char.attr_handle;

            esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
            if (get_attr_ret == ESP_FAIL){
                ESP_LOGE(tag, "ILLEGAL HANDLE on characteristic index %d",i);
            }

            ESP_LOGI(tag, "Characteristic at index=%d created with length = %x\n",i,length);
            for(int i = 0; i < length; i++){
                ESP_LOGI(tag, "prf_char[%x] =%x\n",i,prf_char[i]);
            }
            esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(
                gl_profile.service_handle, 
                &gl_profile.descr_uuid[i],
                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                NULL,
                NULL
            );

            if (add_descr_ret){
                ESP_LOGE(tag, "add char at index=%d descr failed, error code =%x", i,add_descr_ret);
            }
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        ESP_LOGI(tag, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(tag, "SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT: {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        ESP_LOGI(tag, "ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        gl_profile.conn_id = param->connect.conn_id;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        bt_connected = true;
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(tag, "ESP_GATTS_DISCONNECT_EVT, disconnect reason 0x%x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        bt_connected = false;
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(tag, "ESP_GATTS_CONF_EVT, status %d attr_handle %d", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK){
            esp_log_buffer_hex(tag, param->conf.value, param->conf.len);
        }
        break;
    default:
        break;
    }
}


void setup_ble(){
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(tag, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(tag, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(tag, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(tag, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(tag, "gatts register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(tag, "gap register error, error code = %x", ret);
        return;
    }
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret){
        ESP_LOGE(tag, "gatts app register error, error code = %x", ret);
        return;
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(tag, "set local  MTU failed, error code = %x", local_mtu_ret);
    }
}

bool get_char_attr_value(const uint8_t **data,int char_index){
    uint16_t length;
    esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(gl_profile.char_handle[char_index],  &length, data);
    
    if (get_attr_ret == ESP_FAIL){
        ESP_LOGE(tag, "Could not get attribute value (Handle %x)",gl_profile.char_handle[char_index]);
        return false;
    }
    if(length != gl_profile.char_attr[char_index].attr_len){
        ESP_LOGE(tag, "Wrong attribute value length is %d should be %d",length , gl_profile.char_attr[char_index].attr_len);
        return false;
    }

    return true;
}

bool get_dir_status(struct dir_data* out){
    const uint8_t *characteristic_chars;

    bool ret = get_char_attr_value(&characteristic_chars,DIR_CHAR_INDEX);
    out->dir = characteristic_chars[0];

    ret = get_char_attr_value(&characteristic_chars,METERS_CHAR_INDEX);
    out->meters = characteristic_chars[3] | (characteristic_chars[2] << 8) | (characteristic_chars[1] << 16) | (characteristic_chars[0] << 24);
    
    ret = get_char_attr_value(&characteristic_chars,SPEED_CHAR_INDEX);
    out->speed = characteristic_chars[0];

    ret = get_char_attr_value(&characteristic_chars,MODE_CHAR_INDEX);
    out->mode = characteristic_chars[0];

    return ret;
}

bool is_bt_connected(){
    return bt_connected;
}