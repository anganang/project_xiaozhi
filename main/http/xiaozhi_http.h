#ifndef __XIAOZHI_HTTP_H__
#define __XIAOZHI_HTTP_H__

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include <string.h>
#include "stdio.h"
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "esp_tls.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_random.h"
#include "xiaozhi_wifi.h"
#include "cJSON.h"
#include "xiaozhi_data.h"

void xiaozhi_http_client_init(void);

#endif /* __XIAOZHI_HTTP_H__ */
