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
#define METER_DISPLAY_SEG 50

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

void display_turn(uint8_t icon[TURN_PAGE_COUNT][TURN_WIDTH]){
	uint8_t* buf[TURN_PAGE_COUNT];
	for(int i=0;i<TURN_PAGE_COUNT;i++){
		size_t size = TURN_WIDTH * sizeof(uint8_t);
		buf[i] = (uint8_t*) malloc(size);
		memcpy(buf[i],icon[i],size);
	}

	display_partial_image(&dev,buf,0,TURN_PAGE_COUNT,0,TURN_WIDTH);

	for(int i=0;i<TURN_PAGE_COUNT;i++){
		free(buf[i]);
	}
}

void display_numbers(uint8_t numbers[MAX_PAGE_COUNT][NUMBER_WIDTH]){
	uint8_t* buf[MAX_PAGE_COUNT];
	for(int i=0;i<MAX_PAGE_COUNT;i++){
		size_t size = NUMBER_WIDTH * sizeof(uint8_t);
		buf[i] = (uint8_t*) malloc(size);
		memcpy(buf[i],numbers[i],size);
	}

	display_partial_image(&dev,buf,0,MAX_PAGE_COUNT,METER_DISPLAY_SEG,NUMBER_WIDTH);

	for(int i=0;i<MAX_PAGE_COUNT;i++){
		free(buf[i]);
	}
}



void display_meters(uint32_t meters){
	if(meters == 0){
		// display_numbers(numbers[0]);
		return;
	}

	
	uint8_t squashed_numbers[MAX_PAGE_COUNT][NUMBER_WIDTH] = {0};
	int curr_display_pixel = 0;
	while(meters != 0){
		uint32_t digit = meters%10;
		int curr_number_pixel = 0;

		while (curr_number_pixel < NUMBER_IMAGE_WIDTH){
			int curr_display_page = curr_display_pixel/8;
			int curr_number_page = curr_number_pixel/8;

			int now_writing_pixels_count = 8-(curr_display_pixel%8);

			for(int i=0;i<NUMBER_WIDTH;i++){

				uint8_t constant_part = (squashed_numbers[curr_display_page][i] << now_writing_pixels_count) >> now_writing_pixels_count;
				uint8_t new_part = (numbers[digit][curr_number_page][i] >> (8 - now_writing_pixels_count)) << (8 - now_writing_pixels_count);
				squashed_numbers[curr_display_page][i] = new_part | constant_part;
			}

			curr_number_pixel+=now_writing_pixels_count;
			curr_display_pixel+=now_writing_pixels_count;
		}

		meters/=10;
	}
	display_numbers(squashed_numbers);
}

void update_dir_display(struct dir_data* data){
	if(data->dir == TURN_RIGHT){
		ESP_LOGI(tag,"Displaying turn right");
		display_turn(turn_right);
	}else if(data->dir == STRAIGHT){
		ESP_LOGI(tag,"Displaying go straight");
		display_turn(straight);
	}else if(data->dir == TURN_LEFT){	
		ESP_LOGI(tag,"Displaying turn left");
		display_turn(turn_left);
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

int currMeters=98;
void dir_disp_task(void *pvParameter){
    while(1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);

		struct dir_data currDir;
		get_dir_status(&currDir);

		display_meters(currMeters);
		currMeters++;
		currMeters%=110;

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
