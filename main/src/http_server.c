//======================================================================================================================
/// @file       http_server.c
/// @brief      HTTP server
/// @date       2024/7/2
//======================================================================================================================

//======================================================================================================================
// Include definition
//======================================================================================================================
#include <esp_log.h>
#include "esp_camera.h"
#include "esp_http_server.h"

#include "http_server.h"

//======================================================================================================================
// Prototype
//======================================================================================================================
static esp_err_t set_response_headers(httpd_req_t* req);
static esp_err_t send_jpeg_frame(httpd_req_t* req, uint8_t* jpeg_buffer, size_t jpeg_buffer_size);
static esp_err_t process_frame(camera_fb_t* frame_buffer, uint8_t** jpeg_buffer, size_t* jpeg_buffer_size);
static esp_err_t stream_handler(httpd_req_t* req);

//======================================================================================================================
// Private values definition
//======================================================================================================================
static const char* TAG = "HTTP SERVER";

//======================================================================================================================
// Public function
//======================================================================================================================

esp_err_t http_server_start(void) {
  httpd_handle_t camera_httpd = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_uri_t stream_uri = {.uri = "/stream", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL};

  esp_err_t result;
  result = httpd_start(&camera_httpd, &config);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server (%s)", esp_err_to_name(result));
    return result;
  }

  result = httpd_register_uri_handler(camera_httpd, &stream_uri);
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Failed to register URI handler (%s)", esp_err_to_name(result));
    httpd_stop(camera_httpd);
    return result;
  }

  return ESP_OK;
}

//======================================================================================================================
// Private functions
//======================================================================================================================

static esp_err_t set_response_headers(httpd_req_t* req) {
  esp_err_t result = httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  if (result != ESP_OK) return result;

  result = httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  if (result != ESP_OK) return result;

  result = httpd_resp_set_hdr(req, "X-Content-Type-Options", "nosniff");
  if (result != ESP_OK) return result;

  result = httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  return result;
}

static esp_err_t send_jpeg_frame(httpd_req_t* req, uint8_t* jpeg_buffer, size_t jpeg_buffer_size) {
  char part_buffer[64];
  size_t header_length =
      snprintf(part_buffer, sizeof(part_buffer),
               "\r\n--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n", jpeg_buffer_size);

  esp_err_t result = httpd_resp_send_chunk(req, part_buffer, header_length);
  if (result != ESP_OK) return result;

  result = httpd_resp_send_chunk(req, (const char*)jpeg_buffer, jpeg_buffer_size);
  if (result != ESP_OK) return result;

  return httpd_resp_send_chunk(req, "\r\n--frame\r\n", 13);
}

static esp_err_t process_frame(camera_fb_t* frame_buffer, uint8_t** jpeg_buffer, size_t* jpeg_buffer_size) {
  if (frame_buffer->format != PIXFORMAT_JPEG) {
    bool jpeg_converted = frame2jpg(frame_buffer, 80, jpeg_buffer, jpeg_buffer_size);
    if (!jpeg_converted) {
      ESP_LOGE(TAG, "Failed to encode frame to JPEG");
      esp_camera_fb_return(frame_buffer);
      return ESP_FAIL;
    }
  } else {
    *jpeg_buffer_size = frame_buffer->len;
    *jpeg_buffer = frame_buffer->buf;
  }
  return ESP_OK;
}

static esp_err_t stream_handler(httpd_req_t* req) {
  esp_err_t result = set_response_headers(req);
  if (result != ESP_OK) return result;

  while (1) {
    camera_fb_t* frame_buffer = esp_camera_fb_get();
    if (!frame_buffer) {
      ESP_LOGE(TAG, "Failed to capture camera frame");
      result = ESP_FAIL;

    } else {
      uint8_t* jpeg_buffer = NULL;
      size_t jpeg_buffer_size = 0;
      result = process_frame(frame_buffer, &jpeg_buffer, &jpeg_buffer_size);
      if (result == ESP_OK) {
        result = send_jpeg_frame(req, jpeg_buffer, jpeg_buffer_size);
      }

      if (frame_buffer->format == PIXFORMAT_JPEG) {
        esp_camera_fb_return(frame_buffer);
      } else if (jpeg_buffer) {
        free(jpeg_buffer);
      }
    }

    if (result != ESP_OK) {
      if (frame_buffer && frame_buffer->format == PIXFORMAT_JPEG) {
        esp_camera_fb_return(frame_buffer);
      }
      break;
    }
  }
  return result;
}
