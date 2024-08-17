//======================================================================================================================
/// @file       wifi_manager.c
/// @brief      Wi-Fi manager
/// @date       2024/7/2
//======================================================================================================================

//======================================================================================================================
// Include definition
//======================================================================================================================
#include <esp_log.h>
#include "esp_netif.h"
#include "esp_wifi.h"
#include "lwip/ip4_addr.h"
#include "lwip/sockets.h"
#include "mdns.h"
#include "nvs_flash.h"

#include "wifi_manager.h"

//======================================================================================================================
// Constant definition
//======================================================================================================================
// mDNS hostname
#define MDNS_HOSTNAME (CONFIG_MDNS_HOSTNAME)

// AP mode or ST mode
#ifdef CONFIG_WIFI_AP_MODE_ENABLED
#define WIFI_AP_MODE_ENABLED (CONFIG_WIFI_AP_MODE_ENABLED)
#else
#define WIFI_AP_MODE_ENABLED (false)
#endif

// Main board SSID or Sub board SSID
#ifdef CONFIG_WIFI_AP_SUB_SSID_ENABLED
#define WIFI_AP_SSID (CONFIG_WIFI_AP_SUB_SSID)
#else
#define WIFI_AP_SSID (CONFIG_WIFI_AP_MAIN_SSID)
#endif

// WiFi AP parameters
#define WIFI_AP_STATIC_IP (CONFIG_WIFI_AP_STATIC_IP)
#define WIFI_AP_NET_MASK (CONFIG_WIFI_AP_NET_MASK)
#define WIFI_AP_PASSWORD (CONFIG_WIFI_AP_PASSWORD)
#define WIFI_AP_CHANNEL (CONFIG_WIFI_AP_CHANNEL)
#define WIFI_AP_MAX_STA_CONN (CONFIG_WIFI_AP_MAX_STA_CONN)

//======================================================================================================================
// Prototype
//======================================================================================================================
static esp_err_t start_access_point(void);
static esp_err_t start_mdns_service(void);

//======================================================================================================================
// Private values definition
//======================================================================================================================
static const char* TAG = "WIFI";

//======================================================================================================================
// Public function
//======================================================================================================================

esp_err_t wifi_init(void) {
  esp_err_t result;

  // Initialize NVS
  result = nvs_flash_init();
  if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    result = nvs_flash_init();
  }
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize NVS (%s)", esp_err_to_name(result));
    return result;
  }

  // Initialize network interface
  result = esp_netif_init();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize network (%s)", esp_err_to_name(result));
    return result;
  }

  // Create default event loop
  result = esp_event_loop_create_default();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create event loop (%s)", esp_err_to_name(result));
    return result;
  }

  // Start AP mode
  result = start_access_point();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start Wi-Fi AP (%s)", esp_err_to_name(result));
    return result;
  }

  // Start mDNS
  result = start_mdns_service();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start mDNS service (%s)", esp_err_to_name(result));
    return result;
  }

  return ESP_OK;
}

//======================================================================================================================
// Private functions
//======================================================================================================================

static esp_err_t start_access_point(void) {
  esp_netif_t* wifiAP = esp_netif_create_default_wifi_ap();
  esp_netif_ip_info_t ipInfo;
  esp_err_t result;
  inet_pton(AF_INET, WIFI_AP_STATIC_IP, &ipInfo.ip);
  inet_pton(AF_INET, WIFI_AP_NET_MASK, &ipInfo.netmask);
  ipInfo.gw.addr = ipInfo.ip.addr;
  esp_netif_dhcps_stop(wifiAP);
  esp_netif_set_ip_info(wifiAP, &ipInfo);
  esp_netif_dhcps_start(wifiAP);

  // Initialize Wi-Fi
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  result = esp_wifi_init(&cfg);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize Wi-Fi (%s)", esp_err_to_name(result));
    return result;
  }

  // Set AP mode
  result = esp_wifi_set_mode(WIFI_MODE_AP);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to set AP mode (%s)", esp_err_to_name(result));
    return result;
  }

  // Set configuration
  wifi_config_t wifi_config = {
      .ap = {.ssid = WIFI_AP_SSID,
             .ssid_len = strlen(WIFI_AP_SSID),
             .channel = WIFI_AP_CHANNEL,
             .password = WIFI_AP_PASSWORD,
             .max_connection = WIFI_AP_MAX_STA_CONN,
             .authmode = WIFI_AUTH_WPA_WPA2_PSK},
  };
  if (strlen(WIFI_AP_PASSWORD) == 0) {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }
  result = esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure Wi-Fi (%s)", esp_err_to_name(result));
    return result;
  }

  // Start Wi-Fi
  result = esp_wifi_start();
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start Wi-Fi (%s)", esp_err_to_name(result));
    return result;
  }

  ESP_LOGI(TAG, "result start Wi-Fi AP, SSID:%s, channel:%d", WIFI_AP_SSID, WIFI_AP_CHANNEL);
  return ESP_OK;
}

static esp_err_t start_mdns_service(void) {
  // Initialize mDNS service
  esp_err_t result = mdns_init();
  if (result) {
    ESP_LOGE(TAG, "Failed to initialize mDNS (%s)", esp_err_to_name(result));
    return result;
  }

  // Set hostname
  mdns_hostname_set(MDNS_HOSTNAME);
  if (result) {
    ESP_LOGE(TAG, "Failed to set host name (%s)", esp_err_to_name(result));
    return result;
  }

  // Start mDNS
  mdns_instance_name_set("http server");
  if (result) {
    ESP_LOGE(TAG, "Failed to start mDNS service (%s)", esp_err_to_name(result));
    return result;
  }

  return ESP_OK;
}