#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "dir_icons.h"

#include "ssd1306.h"
#include "font8x8_basic.h"
#include "gatts_server.h"

/*
 You have to set this config value with menuconfig
 CONFIG_INTERFACE

 for i2c
 CONFIG_MODEL
 CONFIG_SDA_GPIO
 CONFIG_SCL_GPIO
 CONFIG_RESET_GPIO

 for SPI
 CONFIG_CS_GPIO
 CONFIG_DC_GPIO
 CONFIG_RESET_GPIO
*/
#define tag "SSD1306"

SSD1306_t dev;
int center, top, bottom;
char lineChar[20];
struct dir_data prevDir = {NO_DIR,0};

void clear_display(){
	ssd1306_clear_screen(&dev, false);
	ssd1306_contrast(&dev, 0xff);
}

void config_display(){

	#if CONFIG_I2C_INTERFACE
		ESP_LOGI(tag, "INTERFACE is i2c");
		ESP_LOGI(tag, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
		ESP_LOGI(tag, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
		ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
		i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
	#endif // CONFIG_I2C_INTERFACE

	#if CONFIG_SPI_INTERFACE
		ESP_LOGI(tag, "INTERFACE is SPI");
		ESP_LOGI(tag, "CONFIG_MOSI_GPIO=%d",CONFIG_MOSI_GPIO);
		ESP_LOGI(tag, "CONFIG_SCLK_GPIO=%d",CONFIG_SCLK_GPIO);
		ESP_LOGI(tag, "CONFIG_CS_GPIO=%d",CONFIG_CS_GPIO);
		ESP_LOGI(tag, "CONFIG_DC_GPIO=%d",CONFIG_DC_GPIO);
		ESP_LOGI(tag, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
		spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
	#endif // CONFIG_SPI_INTERFACE

	#if CONFIG_FLIP
		dev._flip = true;
		ESP_LOGW(tag, "Flip upside down");
	#endif

	#if CONFIG_SSD1306_128x64
		ESP_LOGI(tag, "Panel is 128x64");
		ssd1306_init(&dev, 128, 64);
	#endif // CONFIG_SSD1306_128x64
	#if CONFIG_SSD1306_128x32
		ESP_LOGI(tag, "Panel is 128x32");
		ssd1306_init(&dev, 128, 32);
	#endif // CONFIG_SSD1306_128x32

	clear_display();
}

void update_dir_display(struct dir_data* data){
	clear_display();

	if(data->dir == TURN_RIGHT){
		ESP_LOGI(tag,"Displaying turn right");
		display_partial_image(&dev,turn_right,0,8,0,TURN_WIDTH);
	}else if(data->dir == STRAIGHT){
		ESP_LOGI(tag,"Displaying go straight");
		display_partial_image(&dev,straight,0,8,0,TURN_WIDTH);
	}else if(data->dir == TURN_LEFT){	
		ESP_LOGI(tag,"Displaying turn left");
		display_partial_image(&dev,turn_left,0,8,0,TURN_WIDTH);
	}else if(data->dir == NO_DIR){
		ESP_LOGI(tag,"Displaying no dir");
		clear_display();
		return;
	}else{
		ESP_LOGW(tag,"Invalid direction in characteristic value attribute");
	}

	char meters_str[10];
	sprintf(meters_str,"%dm",data->meters);
	ESP_LOGI(tag,"Displaying %d meters",data->meters);
}

void dir_disp_task(void *pvParameter){
    while(1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);

		struct dir_data currDir;
		get_dir_status(&currDir);

		if(prevDir.dir == currDir.dir && prevDir.meters == currDir.meters){
			continue;
		}

        update_dir_display(&currDir);

		prevDir = currDir;
    }
}

void app_main(void)
{
    esp_err_t ret;
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

	setup_ble();

	config_display();
	vTaskDelay(1000 / portTICK_PERIOD_MS);

	xTaskCreate(&dir_disp_task, "display_dir_on_oled", 2048, NULL, 5, NULL);
}
