//======================================================================================================================
/// @file       http_server.h
/// @brief      HTTP server
/// @date       2024/7/2
//======================================================================================================================

#pragma once

//======================================================================================================================
// Include definition
//======================================================================================================================
#include <stdint.h>
#include "esp_err.h"

//======================================================================================================================
// Public function
//======================================================================================================================
#ifdef __cplusplus
extern "C" {
#endif

esp_err_t http_server_start(void);

#ifdef __cplusplus
}
#endif
