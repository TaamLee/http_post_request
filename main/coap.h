#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "driver/gptimer.h"

#include "nvs_flash.h"


#include "coap3/coap.h"


#define COAP_DEFAULT_DEMO_URI "coap://192.168.0.2:5683/control_motor"


extern int temp_value_outdoor;
void coap_client_start(void);
void get_temperature_request_coap(void);
void put_control_request_coap (char *cmd);