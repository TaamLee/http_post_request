#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <arpa/inet.h>
#include "lwip/opt.h"


#include "lwip/err.h"
#include "lwip/sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define ON_COMMAND    "on"
#define OFF_COMMAND    "off"

#include "mqtt.h"
#include "wifi.h"
#include "coap.h"
#include "http.h"

int wifi_connected = 0;
void cmd_handler_task(void *p)
{
  while (1)
  {
    if (wifi_connected == 0)
    {
      //dieu khien theo kich ban co dinh
      //control motor outdoor
      if (temp_value_outdoor > 20)
      {
        printf("outdoor: turn off motor\n");
        put_control_request_coap(OFF_COMMAND);
        vTaskDelay( 2000 / portTICK_PERIOD_MS );
      }
      else
      {
        printf("outdoor: turn on motor\n");
        put_control_request_coap(ON_COMMAND);
        vTaskDelay( 2000 / portTICK_PERIOD_MS );
      }

      //control sock indoor
      if (temp_value_indoor > 28)
      {
        // turn off sock
        printf("indoor: turn off sock\n");
        control_sock(OFF_COMMAND);
        vTaskDelay( 2000 / portTICK_PERIOD_MS );
      }
      else
      {
        // turn on sock
        printf("indoor: turn on sock\n");
        control_sock(ON_COMMAND);
        vTaskDelay( 3000 / portTICK_PERIOD_MS );
      }

    }
    else if (wifi_connected == 1)
    {
      //nhan lenh tu server
    }
  }

}
void get_temperature_task (void* p)
{
  while (1)
  {
    // get_outdoor_temperature from device 1 through Coap
      get_temperature_request_coap();
      vTaskDelay( 2000 / portTICK_PERIOD_MS );
  }

}
void app_main(void)
{
    // init flash
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    //start ap_sta_mode wifi
    ap_sta_wifi_start();

    //wait until wifi establish completely
    //start mqtt protocol
    mqtt_app_start();
    //start coap client
    coap_client_start();
    
    xTaskCreatePinnedToCore( cmd_handler_task, "cmd_handler", 4*1024, NULL,5,NULL, 1 );
    xTaskCreatePinnedToCore( get_temperature_task, "get_temp_task", 4*1024, NULL,5,NULL, 1 );
    
}
