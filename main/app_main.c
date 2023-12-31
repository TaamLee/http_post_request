
#include "app_main.h"
#include "mqtt.h"
#include "wifi.h"
#include "coap.h"
#include "http.h"

int wifi_connected = 0;
Data_Information_t temp_value_indoor , temp_value_outdoor;
QueueHandle_t data_queue;
TaskHandle_t cmd_handler_task_Handle;
void cmd_handler_task(void *p)
{
  uint32_t ulNotifiedValue;
  while (1)
  {
    if (wifi_connected == 0)
    {
        // wifi disconnected
        // wait notify, Task will unblock as soon as the temperature value arrives
        xTaskNotifyWait(0x00,0xffffffff,&ulNotifiedValue,portMAX_DELAY);
        if( ( ulNotifiedValue & 0x01 ) != 0 )
        {
            // notify from Device 1 (Coap) => check temperature
            if (temp_value_outdoor.temperature_value > 20)
            {
                printf("outdoor: turn off motor\n");
                ControlMotor_Coap(OFF_COMMAND);
                //vTaskDelay( 3000 / portTICK_PERIOD_MS );
            }
            else
            {
                printf("outdoor: turn on motor\n");
                ControlMotor_Coap(ON_COMMAND);
                //vTaskDelay( 3000 / portTICK_PERIOD_MS );
            }
        }
        else if ( ( ulNotifiedValue & 0x02 ) != 0 )
        {
            // notify from Device 2 (MQTT) => check temperature
            if (temp_value_indoor.temperature_value > 20)
            {
                // turn off sock
                printf("indoor: turn off sock\n");
                ControlSock_Mqtt(OFF_COMMAND);
                //vTaskDelay( 3000 / portTICK_PERIOD_MS );
            }
            else
            {
                // turn on sock
                printf("indoor: turn on sock\n");
                ControlSock_Mqtt(ON_COMMAND);
                //vTaskDelay( 3000 / portTICK_PERIOD_MS );
            }

        }
    }
    else if (wifi_connected == 1)
    {
      //nhan lenh tu server
      vTaskDelay( 3000 / portTICK_PERIOD_MS );
    }
  }

}
void get_temperature_from_device1 (void* p)
{
  while (1)
  {
    // get_outdoor_temperature from device 1 through Coap
      vTaskDelay( 500 / portTICK_PERIOD_MS );
      GetTemperature_Coap();
      vTaskDelay( 3500 / portTICK_PERIOD_MS );
  }

}
void send_to_thingsboard_task (void* p)
{
  Data_Information_t data_dequeue;
  while (1)
  {
    // send data to thingsboard
      if( xQueueReceive( data_queue,&data_dequeue,portMAX_DELAY ) == pdPASS )
      {
        if (data_dequeue.ID_DEVICE == 1)
        {
          // notify to cmdhandler task
          xTaskNotify(cmd_handler_task_Handle,( 1UL << 0UL ),eSetBits );
          // temperature from device 1
          SendtoThingsboard_Device1(data_dequeue.temperature_value);
        } 
        else if (data_dequeue.ID_DEVICE == 2)
        {
          // notify to cmdhandler task
          xTaskNotify(cmd_handler_task_Handle,( 1UL << 1UL ),eSetBits );
          // temperature from device 2
          SendtoThingsboard_Device2(data_dequeue.temperature_value);
        }
      }
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

    //create a data queue
    data_queue = xQueueCreate( 10, sizeof (Data_Information_t) );

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    //start ap_sta_mode wifi
    ap_sta_wifi_start();

    //wait until wifi establish completely
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    SendtoThingsboard_Device1(15);
    SendtoThingsboard_Device2(15);
    //start mqtt protocol
    mqtt_app_start();
    
    xTaskCreatePinnedToCore( cmd_handler_task, "cmd_handler", 4*1024, NULL,5,&cmd_handler_task_Handle, 1 );
    xTaskCreatePinnedToCore( get_temperature_from_device1, "get_temp_task", 4*1024, NULL,5,NULL, 1 );
    xTaskCreatePinnedToCore( send_to_thingsboard_task, "send_to_thingsboard_task", 4*1024, NULL,5,NULL, 1 );
}