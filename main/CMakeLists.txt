idf_component_register(
    SRCS
        "src/main.c"
        "src/camera_driver.c"
        "src/wifi_manager.c"
        "src/http_server.c"
        "src/sdcard_fatfs_manager.c"
    
    INCLUDE_DIRS 
        "."
    
    REQUIRES 
        esp32-camera 
        esp_wifi
        esp_http_server
        esp_timer
        nvs_flash
        fatfs
)
