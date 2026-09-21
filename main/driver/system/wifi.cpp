#include "wifi.hpp"
#include "esp_log.h"
#include <cstring>
using namespace qb::driver;

const char* WiFi::TAG = "WiFi";

EventGroupHandle_t WiFi::wifi_event_group;

wifi_config_t WiFi::sta_config;
wifi_config_t WiFi::ap_config;

WiFi::STA WiFi::sta = {
    .connect_retry = 0,
    .connected = false,
    .got_ip = false,
    .is_connecting = false,
    .bssid = "000000",
    .ssid = ""
};
WiFi::AP WiFi::ap = {
    .sta_list = {}
};
WiFi::Config WiFi::config = {
    .sta_auto_connect = false,
    .sta_connect_max_retries = 3,
};

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

void WiFi::connect(void)
{
    ESP_LOGI(TAG, "Connecting...\n");
    sta.connect_retry = 0;
    sta.is_connecting = (esp_wifi_connect() == ESP_OK);
}

bool WiFi::reconnect(void)
{
    if (sta.is_connecting && sta.connect_retry++ < config.sta_connect_max_retries) {
        ESP_LOGI(TAG, "Reconnecting...\n");
        esp_err_t ret = esp_wifi_connect();
        sta.is_connecting = (ret == ESP_OK);
        return true;
    }
    return false;
}

void WiFi::set_mode(wifi_mode_t mode) {
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
}

void WiFi::set_sta_bssid(const std::string& bssid) {
    memcpy((char*) sta_config.sta.bssid, bssid.c_str(), 6);
    sta_config.sta.bssid_set = 1;
    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
}

void WiFi::set_sta_ssid(const std::string& ssid) {
    auto n = ssid.size();
    strncpy((char*) sta_config.sta.ssid, ssid.c_str(), n);
    sta_config.sta.ssid[n] = '\0';
    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
}

void WiFi::set_sta_passwd(const std::string& passwd) {
    auto n = passwd.size();
    strncpy((char*) sta_config.sta.password, passwd.c_str(), n);
    sta_config.sta.password[n] = '\0';
    
    WiFi::sta_config.sta.threshold.authmode = QUIMBLOS_WIFI_SCAN_AUTH_MODE_THRESHOLD;
    /* 802.11w: mandatory for WPA3 and for APs that require PMF; "required = false" keeps
     * the WPA2 fallback working on WPA2/WPA3 transition-mode APs. */
    WiFi::sta_config.sta.pmf_cfg.capable = true;
    WiFi::sta_config.sta.pmf_cfg.required = false;
    /* Accept both SAE methods, otherwise H2E-only WPA3 APs are rejected (reason 210). */
    WiFi::sta_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
}


void WiFi::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    wifi_event_sta_connected_t *event;
    wifi_event_sta_disconnected_t *disconnected_event;
    wifi_mode_t mode;

    switch (event_id) {

    /* STA */

    case WIFI_EVENT_STA_START:
        if (WiFi::config.sta_auto_connect) WiFi::connect();
        break;
    case WIFI_EVENT_STA_CONNECTED:
        WiFi::sta.connected = true;
        WiFi::sta.is_connecting = false;
        event = (wifi_event_sta_connected_t*) event_data;
        WiFi::sta.bssid.assign((char*) event->bssid, 6);
        WiFi::sta.ssid.assign((char*) event->ssid, event->ssid_len);
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        disconnected_event = (wifi_event_sta_disconnected_t*) event_data;
        
        /* 
            [disconnected_event->reason]
            2 AUTH_EXPIRE
            15/204 HANDSHAKE_TIMEOUT
            202 AUTH_FAIL
            203 ASSOC_FAIL
            205 CONNECTION_FAIL
            210 incompatible security
            211 authmode threshold
        */
        ESP_LOGI(WiFi::TAG, "BLUFI wifi disconnected, reason=%d rssi=%d\n",
                   disconnected_event->reason, disconnected_event->rssi);

        if (WiFi::sta.connected == false && reconnect() == false) {
            WiFi::sta.is_connecting = false;
            // TODO
            // example_record_wifi_conn_info(disconnected_event->rssi, disconnected_event->reason);
        }
        
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        WiFi::sta.connected = false;
        WiFi::sta.got_ip = false;
        WiFi::sta.bssid.clear();
        WiFi::sta.ssid.clear();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;

    #ifdef CONFIG_ESP_WIFI_SOFTAP_SUPPORT

    // TODO: Review this (Soft-AP currently not in use)

    case WIFI_EVENT_AP_START:
        esp_wifi_get_mode(&mode);

        /* TODO: get config or information of softap, then set to report extra_info */
        if (ble_is_connected == true) {
            if (WiFi::sta.connected) {
                esp_blufi_extra_info_t info;
                memset(&info, 0, sizeof(esp_blufi_extra_info_t));
                memcpy(info.sta.bssid, WiFi::sta.bssid, 6);
                info.sta_bssid_set = true;
                info.sta.ssid = WiFi::sta.ssid;
                info.sta_ssid_len = WiFi::sta.ssid_len;
                esp_blufi_send_wifi_conn_report(mode, WiFi::sta.got_ip ? ESP_BLUFI_STA_CONN_SUCCESS : ESP_BLUFI_STA_NO_IP, softap_get_current_connection_number(), &info);
            } else if (WiFi::sta.is_connecting) {
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONNECTING, softap_get_current_connection_number(), &WiFi::sta.conn_info);
            } else {
                esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_FAIL, softap_get_current_connection_number(), &WiFi::sta.conn_info);
            }
        } else {
            ESP_LOGI(WiFi::TAG, "BLUFI BLE is not connected yet\n");
        }
        break;
    case WIFI_EVENT_SCAN_DONE: {
        uint16_t apCount = 0;
        esp_wifi_scan_get_ap_num(&apCount);
        if (apCount == 0) {
            ESP_LOGI(WiFi::TAG, "Nothing AP found");
            break;
        }
        wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
        if (!ap_list) {
            BLUFI_ERROR("malloc error, ap_list is NULL");
            esp_wifi_clear_ap_list();
            break;
        }
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_list));
        esp_blufi_ap_record_t * blufi_ap_list = (esp_blufi_ap_record_t *)malloc(apCount * sizeof(esp_blufi_ap_record_t));
        if (!blufi_ap_list) {
            if (ap_list) {
                free(ap_list);
            }
            BLUFI_ERROR("malloc error, blufi_ap_list is NULL");
            break;
        }
        for (int i = 0; i < apCount; ++i)
        {
            blufi_ap_list[i].rssi = ap_list[i].rssi;
            memcpy(blufi_ap_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid));
        }

        if (ble_is_connected == true) {
            esp_blufi_send_wifi_list(apCount, blufi_ap_list);
        } else {
            ESP_LOGI(WiFi::TAG, "BLUFI BLE is not connected yet\n");
        }

        esp_wifi_scan_stop();
        free(ap_list);
        free(blufi_ap_list);
        break;
    }
    case WIFI_EVENT_AP_STACONNECTED: {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(WiFi::TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
        break;
    }
    case WIFI_EVENT_AP_STADISCONNECTED: {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(WiFi::TAG, "station "MACSTR" leave, AID=%d, reason=%d", MAC2STR(event->mac), event->aid, event->reason);
        break;
    }

    #endif // CONFIG_ESP_WIFI_SOFTAP_SUPPORT

    default:
        break;
    }
    return;
}

void WiFi::ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    // wifi_mode_t mode;

    // switch (event_id) {
    // case IP_EVENT_STA_GOT_IP: {
    //     esp_blufi_extra_info_t info;

    //     xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    //     esp_wifi_get_mode(&mode);

    //     memset(&info, 0, sizeof(esp_blufi_extra_info_t));
    //     memcpy(info.sta.bssid, WiFi::sta.bssid, 6);
    //     info.sta_bssid_set = true;
    //     info.sta.ssid = WiFi::sta.ssid;
    //     info.sta_ssid_len = WiFi::sta.ssid_len;
    //     WiFi::sta.got_ip = true;
    //     if (ble_is_connected == true) {
    //         esp_blufi_send_wifi_conn_report(mode, ESP_BLUFI_STA_CONN_SUCCESS, softap_get_current_connection_number(), &info);
    //     } else {
    //         ESP_LOGI(WiFi::TAG, "BLUFI BLE is not connected yet\n");
    //     }
    //     break;
    // }
    // default:
    //     break;
    // }
    return;
}