/*
 *  main.c
 *
 *  Created on: 29 mai 2024
 *      Author: Guillaume Varlet
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "sdkconfig.h"

#include "wifi/wifi_connection.h"
#include "front_panel/front_light.h"

extern "C" void app_main(void)
{
	/******************************************************************************************************************
	 *												Configurations													  *
	 *****************************************************************************************************************/
    WifiStation station;
	configure_front_light();

	/******************************************************************************************************************
	 *												Main loop													  	  *
	 *****************************************************************************************************************/
	while (true)
	{
		/* update_front_light(45, green, 60); */
		vTaskDelay(200/portTICK_PERIOD_MS);
	}
	
}
