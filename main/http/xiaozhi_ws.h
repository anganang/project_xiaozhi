#ifndef __XIAOZHI_WS_H__
#define __XIAOZHI_WS_H__

#include "string.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <esp_websocket_client.h>
#include "ctype.h"
#include "esp_crt_bundle.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "xiaozhi_data.h"

void xiaozhi_ws_init(void);
void xiaozhi_ws_send_text(char *text);
void xiaozhi_ws_send_opus(uint8_t *data, size_t len);
void xiaozhi_ws_check_wakeup(void);

#endif /* __XIAOZHI_WS_H__ */
