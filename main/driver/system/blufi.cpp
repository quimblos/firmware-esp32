#include "blufi.hpp"
#include "esp_blufi.h"

using namespace qb::driver;

WiFi* BluFi::wifi;
const char* BluFi::TAG = "BluFi";

esp_blufi_callbacks_t BluFi::callbacks = {
    .event_cb = BluFi::event_callback,
    .negotiate_data_handler = BluFi::dh_negotiate_data_handler,
    .encrypt_func = BluFi::aes_encrypt,
    .decrypt_func = BluFi::aes_decrypt,
    .checksum_func = BluFi::crc_checksum
};

bool BluFi::ble_is_connected = false;
esp_blufi_extra_info_t BluFi::sta_conn_info;

char BluFi::device_name[] = "BLUFI_" "QUIMBLOS";

#ifdef CONFIG_BT_BLUEDROID_ENABLED
esp_err_t esp_blufi_host_init(void)
{
    int ret;
    esp_bluedroid_config_t cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    ret = esp_bluedroid_init_with_cfg(&cfg);
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s init bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s init bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }
    ESP_LOGI(BluFi::TAG, "BD ADDR: " ESP_BD_ADDR_STR "\n", ESP_BD_ADDR_HEX(esp_bt_dev_get_address()));

    /* Set the default device name */
    ret = esp_ble_gap_set_device_name(BluFi::device_name);
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s set device name failed: %s\n", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    return ESP_OK;

}

#ifdef CONFIG_QUIMBLOS_BLUFI_BLE_SMP_ENABLE

void esp_blufi_set_ble_security_params(void)
{
    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM; // Secure Connections with MITM protection (no bonding)
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;               // IO capability: DisplayOnly
    uint8_t key_size = 16;      //the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    //set static passkey
    uint32_t passkey = 123456;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    ESP_LOGI(BluFi::TAG, "BLE SMP passkey: %06" PRIu32 " (WARNING: Change this default value for production or don't use static passkey!)\n", passkey);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t));
    /* If your BLE device acts as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the master;
    If your BLE device acts as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

#endif // #if CONFIG_QUIMBLOS_BLUFI_BLE_SMP_ENABLE


esp_err_t BluFi::gap_register_callback(void)
{
   int rc;
   rc = esp_ble_gap_register_callback(esp_blufi_gap_event_handler);
    if(rc){
        return rc;
    }
    return esp_blufi_profile_init();
}

esp_err_t BluFi::init_host(esp_blufi_callbacks_t *example_callbacks)
{
    esp_err_t ret = ESP_OK;

    ret = esp_blufi_host_init();
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s initialise host failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_blufi_register_callbacks(example_callbacks);
    if(ret){
        ESP_LOGE(BluFi::TAG, "%s blufi register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    ret = gap_register_callback();
    if(ret){
        ESP_LOGE(BluFi::TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    #ifdef CONFIG_QUIMBLOS_BLUFI_BLE_SMP_ENABLE
    esp_blufi_set_ble_security_params();
    #endif // CONFIG_QUIMBLOS_BLUFI_BLE_SMP_ENABLE

    return ESP_OK;
}

esp_err_t BluFi::deinit_host(void)
{
    int ret;
    ret = esp_blufi_profile_deinit();
    if(ret != ESP_OK) {
        return ret;
    }

    ret = esp_bluedroid_disable();
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s deinit bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ret = esp_bluedroid_deinit();
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s deinit bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return ESP_FAIL;
    }

    return ESP_OK;

}

#endif /* CONFIG_BT_BLUEDROID_ENABLED */

#if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
esp_err_t BluFi::init_controller() {
    esp_err_t ret = ESP_OK;
#if CONFIG_IDF_TARGET_ESP32
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
#endif

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s initialize bt controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s enable bt controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }
    return ret;
}
#endif

#if CONFIG_BT_CONTROLLER_ENABLED || !CONFIG_BT_NIMBLE_ENABLED
esp_err_t BluFi::deinit_controller() {
    esp_err_t ret = ESP_OK;
    ret = esp_bt_controller_disable();
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s disable bt controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    ret = esp_bt_controller_deinit();
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s deinit bt controller failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    return ret;
}
#endif

#ifdef CONFIG_BT_NIMBLE_ENABLED
void ble_store_config_init(void);
static void blufi_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void blufi_on_sync(void)
{
  esp_blufi_profile_init();
}

void bleprph_host_task(void *param)
{
    ESP_LOGI("BLUFI_EXAMPLE", "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

esp_err_t esp_blufi_host_init(void)
{
    esp_err_t err;
    err = esp_nimble_init();
    if (err) {
        ESP_LOGE(BluFi::TAG, "%s failed: %s\n", __func__, esp_err_to_name(err));
        return ESP_FAIL;
    }

/* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = blufi_on_reset;
    ble_hs_cfg.sync_cb = blufi_on_sync;
    ble_hs_cfg.gatts_register_cb = esp_blufi_gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* FALSE POSITIVE: BLUFI uses its own app-layer security (DH + AES), not BLE SM.
     * sm_mitm/sm_sc/sm_bonding are opt-in via Kconfig to let the example show
     * multiple configurations; Just Works is acceptable for BLUFI provisioning. */
    ble_hs_cfg.sm_io_cap = 4;
#ifdef CONFIG_QUIMBLOS_BONDING
    ble_hs_cfg.sm_bonding = 1;
#endif
#ifdef CONFIG_QUIMBLOS_MITM
    ble_hs_cfg.sm_mitm = 1;
#endif
#ifdef CONFIG_QUIMBLOS_USE_SC
    ble_hs_cfg.sm_sc = 1;
#else
    ble_hs_cfg.sm_sc = 0;
#ifdef CONFIG_QUIMBLOS_BONDING
    ble_hs_cfg.sm_our_key_dist = 1;
    ble_hs_cfg.sm_their_key_dist = 1;
#endif
#endif

    int rc;
    rc = esp_blufi_gatt_svr_init();
    assert(rc == 0);

#if CONFIG_BT_NIMBLE_GAP_SERVICE
    /* Set the default device name. */
    rc = ble_svc_gap_device_name_set(BluFi::device_name);
    assert(rc == 0);
#endif

    /* XXX Need to have template for store */
    ble_store_config_init();

    esp_blufi_btc_init();

    err = esp_nimble_enable(bleprph_host_task);
    if (err) {
        ESP_LOGE(BluFi::TAG, "%s failed: %s\n", __func__, esp_err_to_name(err));
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t esp_blufi_host_deinit(void)
{
    esp_err_t ret = ESP_OK;

    ret = nimble_port_stop();
    if (ret != ESP_OK) {
        return ret;
    }

    esp_blufi_gatt_svr_deinit();

    if (ret == 0) {
        esp_nimble_deinit();
    }

    ret = esp_blufi_profile_deinit();
    if (ret != ESP_OK) {
        return ret;
    }

    esp_blufi_btc_deinit();

    return ret;
}

esp_err_t esp_blufi_gap_register_callback(void)
{
    return ESP_OK;
}

esp_err_t esp_blufi_host_and_cb_init(esp_blufi_callbacks_t *example_callbacks)
{
    esp_err_t ret = ESP_OK;

    ret = esp_blufi_register_callbacks(example_callbacks);
    if(ret){
        ESP_LOGE(BluFi::TAG, "%s blufi register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    ret = esp_blufi_gap_register_callback();
    if(ret){
        ESP_LOGE(BluFi::TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    ret = esp_blufi_host_init();
    if (ret) {
        ESP_LOGE(BluFi::TAG, "%s initialise host failed: %s\n", __func__, esp_err_to_name(ret));
        return ret;
    }

    return ret;
}

#endif /* CONFIG_BT_NIMBLE_ENABLED */


void BluFi::event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param)
{
    /* actually, should post to blufi_task handle the procedure,
     * now, as a example, we do it more simply */
    switch (event) {

    // Blufi

    case ESP_BLUFI_EVENT_INIT_FINISH:
        ESP_LOGI(BluFi::TAG, "init finish\n");
        #if SOC_MPI_SUPPORTED
            esp_blufi_adv_start();
        #endif
        break;
    case ESP_BLUFI_EVENT_DEINIT_FINISH:
        ESP_LOGI(BluFi::TAG, "deinit finish\n");
        break;
    case ESP_BLUFI_EVENT_REPORT_ERROR:
        ESP_LOGE(BluFi::TAG, "code %d\n", param->report_error.state);
        esp_blufi_send_error_info(param->report_error.state);
        break;
    case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
        ESP_LOGI(BluFi::TAG, "gatt connection closed");
        esp_blufi_disconnect();
        break;

    // BLE

    case ESP_BLUFI_EVENT_BLE_CONNECT:
        ESP_LOGI(BluFi::TAG, "ble connect\n");
        ble_is_connected = true;
        esp_blufi_adv_stop();
        BluFi::init_security();

        #ifdef CONFIG_QUIMBLOS_BLUFI_BLE_SMP_ENABLE
        // Try to initiate BLE security request after connection established.
        ESP_LOGI(BluFi::TAG, "Try to initiate BLE security request\n");
        esp_err_t ret = esp_blufi_start_security_request(param->connect.remote_bda);
        if (ret != ESP_OK) {
            ESP_LOGE(BluFi::TAG, "Failed to start security request: %s\n", esp_err_to_name(ret));
        }
        #endif // CONFIG_QUIMBLOS_BLUFI_BLE_SMP_ENABLE
        break;
    case ESP_BLUFI_EVENT_BLE_DISCONNECT:
        ESP_LOGI(BluFi::TAG, "ble disconnect\n");
        ble_is_connected = false;
        BluFi::deinit_security();
        #if !SOC_MPI_SUPPORTED
            BluFi::dh_pregen_start_with_cb(esp_blufi_adv_start);
        #else
            esp_blufi_adv_start();
        #endif
        break;

    // Wi-Fi

    case ESP_BLUFI_EVENT_SET_WIFI_OPMODE:
        ESP_LOGI(BluFi::TAG, "Set Wi-Fi opmode %d\n", param->wifi_mode.op_mode);
        wifi->set_mode(param->wifi_mode.op_mode);
        break;
    
    case ESP_BLUFI_EVENT_GET_WIFI_STATUS: {
        wifi_mode_t mode;
        esp_blufi_extra_info_t info;

        esp_wifi_get_mode(&mode);

        if (wifi->sta.connected) {
            memset(&info, 0, sizeof(esp_blufi_extra_info_t));
            memcpy(info.sta_bssid, wifi->sta.bssid.c_str(), 6);
            info.sta_bssid_set = true;
            memcpy(info.sta_ssid, wifi->sta.ssid.c_str(), wifi->sta.ssid.size());
            info.sta_ssid_len = wifi->sta.ssid.size();

            #ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
                esp_blufi_send_wifi_conn_report(mode, wifi->sta.got_ip ? ESP_BLUFI_STA_CONN_SUCCESS : ESP_BLUFI_STA_NO_IP, softap_get_current_connection_number(), &info);
            #else
                esp_blufi_send_wifi_conn_report(mode, wifi->sta.got_ip ? ESP_BLUFI_STA_CONN_SUCCESS : ESP_BLUFI_STA_NO_IP, 0xFF, &info);
            #endif
        } else if (wifi->sta.is_connecting) {
            #ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONNECTING, softap_get_current_connection_number(), &wifi->sta.conn_info);
            #else
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONNECTING, 0xFF, &sta_conn_info);
            #endif
        } else {
            #ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
            esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, softap_get_current_connection_number(), &sta_conn_info);
            #else
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, 0xFF, &sta_conn_info);
            #endif
        }
        ESP_LOGI(BluFi::TAG, "BLUFI get wifi status from AP\n");

        break;
    }

    case ESP_BLUFI_EVENT_GET_WIFI_LIST:{
        wifi_scan_config_t config = {
            .ssid = NULL,
            .bssid = NULL,
            .channel = 0,
            .show_hidden = false,
            .scan_type = WIFI_SCAN_TYPE_ACTIVE,
            .scan_time = {
                .active = {
                    .min = 0,
                    .max = 0
                },
                .passive = 0
            },
            .home_chan_dwell_time = 0,
            .channel_bitmap = {
                .ghz_2_channels = 0,
                .ghz_5_channels = 0
            },
            .coex_background_scan = false
        };
        esp_err_t ret = esp_wifi_scan_start(&config, true);
        if (ret != ESP_OK) {
            esp_blufi_send_error_info(ESP_BLUFI_WIFI_SCAN_FAIL);
        }
        break;
    }

    // Wi-Fi > AP

    #ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT
        case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
            ESP_LOGI(BluFi::TAG, "BLUFI request wifi connect to AP\n");
            /* there is no wifi callback when the device has already connected to this wifi
            so disconnect wifi before connection.
            */
            esp_wifi_disconnect();
            
            // TODO
            // example_wifi_connect();

            break;
        case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
            ESP_LOGI(BluFi::TAG, "BLUFI request wifi disconnect from AP\n");
            esp_wifi_disconnect();
            break;
        case ESP_BLUFI_EVENT_RECV_SOFTAP_SSID:
            if (param->softap_ssid.ssid_len >= sizeof(WiFi::ap_config.ap.ssid)/sizeof(WiFi::ap_config.ap.ssid[0])) {
                esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
                ESP_LOGI(BluFi::TAG, "Invalid SOFTAP SSID\n");
                break;
            }
            strncpy((char *)WiFi::ap_config.ap.ssid, (char *)param->softap_ssid.ssid, param->softap_ssid.ssid_len);
            WiFi::ap_config.ap.ssid[param->softap_ssid.ssid_len] = '\0';
            WiFi::ap_config.ap.ssid_len = param->softap_ssid.ssid_len;
            esp_wifi_set_config(WIFI_IF_AP, &WiFi::ap_config);
            ESP_LOGI(BluFi::TAG, "Recv SOFTAP SSID %s, ssid len %d\n", WiFi::ap_config.ap.ssid, WiFi::ap_config.ap.ssid_len);
            break;
        case ESP_BLUFI_EVENT_RECV_SOFTAP_PASSWD:
            if (param->softap_passwd.passwd_len >= sizeof(WiFi::ap_config.ap.password)/sizeof(WiFi::ap_config.ap.password[0])) {
                esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
                ESP_LOGI(BluFi::TAG, "Invalid SOFTAP PASSWD\n");
                break;
            }
            strncpy((char *)WiFi::ap_config.ap.password, (char *)param->softap_passwd.passwd, param->softap_passwd.passwd_len);
            WiFi::ap_config.ap.password[param->softap_passwd.passwd_len] = '\0';
            esp_wifi_set_config(WIFI_IF_AP, &WiFi::ap_config);
            ESP_LOGI(BluFi::TAG, "Recv SOFTAP PASSWORD %s len = %d\n", WiFi::ap_config.ap.password, param->softap_passwd.passwd_len);
            break;
        case ESP_BLUFI_EVENT_RECV_SOFTAP_MAX_CONN_NUM:
            if (param->softap_max_conn_num.max_conn_num > 4) {
                return;
            }
            WiFi::ap_config.ap.max_connection = param->softap_max_conn_num.max_conn_num;
            esp_wifi_set_config(WIFI_IF_AP, &WiFi::ap_config);
            ESP_LOGI(BluFi::TAG, "Recv SOFTAP MAX CONN NUM %d\n", WiFi::ap_config.ap.max_connection);
            break;
        case ESP_BLUFI_EVENT_RECV_SOFTAP_AUTH_MODE:
            if (param->softap_auth_mode.auth_mode >= WIFI_AUTH_MAX) {
                return;
            }
            WiFi::ap_config.ap.authmode = param->softap_auth_mode.auth_mode;
            esp_wifi_set_config(WIFI_IF_AP, &WiFi::ap_config);
            ESP_LOGI(BluFi::TAG, "Recv SOFTAP AUTH MODE %d\n", WiFi::ap_config.ap.authmode);
            break;
        case ESP_BLUFI_EVENT_RECV_SOFTAP_CHANNEL:
            if (param->softap_channel.channel > 13) {
                return;
            }
            WiFi::ap_config.ap.channel = param->softap_channel.channel;
            esp_wifi_set_config(WIFI_IF_AP, &WiFi::ap_config);
            ESP_LOGI(BluFi::TAG, "Recv SOFTAP CHANNEL %d\n", WiFi::ap_config.ap.channel);
            break;
    #endif // CONFIG_ESP_WIFI_SOFTAP_SUPPORT

    // Wi-Fi > STA
    
    case ESP_BLUFI_EVENT_DEAUTHENTICATE_STA:
        /* TODO */
        break;
	case ESP_BLUFI_EVENT_RECV_STA_BSSID:
        ESP_LOGI(BluFi::TAG, "recv STA BSSID %s\n", param->sta_bssid.bssid);
        wifi->set_sta_bssid(std::string((const char*) param->sta_bssid.bssid, 6));
        break;
	case ESP_BLUFI_EVENT_RECV_STA_SSID:
        if (param->sta_ssid.ssid_len < 1 || param->sta_ssid.ssid_len >= 32) {
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
            ESP_LOGE(BluFi::TAG, "Invalid STA SSID\n");
            break;
        }
        wifi->set_sta_ssid(std::string((char*) param->sta_ssid.ssid, param->sta_ssid.ssid_len));
        ESP_LOGI(BluFi::TAG, "recv STA SSID %s\n", WiFi::sta_config.sta.ssid);
        break;
	case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
        ESP_LOGI(BluFi::TAG, "passwd_len=%d passwd=%p", param->sta_passwd.passwd_len, param->sta_passwd.passwd);
        if (param->sta_passwd.passwd_len >= 32) {
            esp_blufi_send_error_info(ESP_BLUFI_DATA_FORMAT_ERROR);
            ESP_LOGI(BluFi::TAG, "Invalid STA PASSWORD\n");
            break;
        }
        if (param->sta_passwd.passwd == NULL || param->sta_passwd.passwd_len == 0) {
            wifi->set_sta_passwd(std::string());                 // empty password
        } else {
            wifi->set_sta_passwd(std::string((char*) param->sta_passwd.passwd, param->sta_passwd.passwd_len));
        }
        ESP_LOGI(BluFi::TAG, "recv STA PASSWORD %s\n", WiFi::sta_config.sta.password);
        break;
        
    // Custom Data
    
    case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
        ESP_LOGI(BluFi::TAG, "recv Custom Data %" PRIu32 "\n", param->custom_data.data_len);
        ESP_LOG_BUFFER_HEX("Custom Data", param->custom_data.data, param->custom_data.data_len);
        break;
	case ESP_BLUFI_EVENT_RECV_USERNAME:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CA_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
        /* Not handle currently */
        break;
	case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
        /* Not handle currently */
        break;;
	case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
        /* Not handle currently */
        break;
    default:
        break;
    }
}