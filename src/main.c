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
#define METER_DISPLAY_SEG 70

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

void display_turn(uint8_t icon[MANEUVER_PAGE_COUNT][MANEUVER_WIDTH]){
	uint8_t* buf[MANEUVER_PAGE_COUNT];
	for(int i=0;i<MANEUVER_PAGE_COUNT;i++){
		size_t size = MANEUVER_WIDTH * sizeof(uint8_t);
		buf[i] = (uint8_t*) malloc(size);
		memcpy(buf[i],icon[i],size);
	}

	display_partial_image(&dev,buf,0,MANEUVER_PAGE_COUNT,0,MANEUVER_WIDTH);

	for(int i=0;i<MANEUVER_PAGE_COUNT;i++){
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

void write_number_icon(uint8_t dest[MAX_PAGE_COUNT][NUMBER_WIDTH],uint8_t icon[NUMBER_PAGE_COUNT][NUMBER_WIDTH],int display_pixel,int number_pixel){
	int number_page = number_pixel/8;
	int display_page = display_pixel/8;

	// Write pixels from start pixel to the end of current page
	if(display_pixel%8!=0){
		int pixels_to_full_page_display = 8-(display_pixel%8);
		int pixels_to_full_page_number = 8-(number_pixel%8);


		for(int seg=0;seg<NUMBER_WIDTH;seg++){

			uint8_t old_right = (dest[display_page][seg] << pixels_to_full_page_display) >> pixels_to_full_page_display;
			uint8_t new_left;

			if(pixels_to_full_page_number < pixels_to_full_page_display){
				uint8_t curr_right = icon[number_page][seg] >> number_pixel%8;
				uint8_t next_left = icon[number_page+1][seg] << pixels_to_full_page_number;

				new_left = (next_left | curr_right)<< display_pixel%8;
			}else{
				new_left = (icon[number_page][seg] >> number_pixel%8) << (number_pixel%8+(pixels_to_full_page_number-pixels_to_full_page_display));
			}

			dest[display_page][seg] = new_left | old_right;
		}

		number_pixel+=pixels_to_full_page_display;
		display_pixel+=pixels_to_full_page_display;
		number_page = number_pixel/8;
		display_page = display_pixel/8;
	}

	// Write full pages
	while (number_pixel < NUMBER_PAGE_COUNT*8-8+1){
		for(int seg=0;seg<NUMBER_WIDTH;seg++){
			int pixels_to_full_page_number = 8-(number_pixel%8);

			uint8_t new_display_page;
			if(pixels_to_full_page_number < 8){
				uint8_t curr_right = icon[number_page][seg] >> number_pixel%8;
				uint8_t next_left = icon[number_page+1][seg] << pixels_to_full_page_number;

				new_display_page = next_left | curr_right;
			}else{
				new_display_page = icon[number_page][seg];
			}

			dest[display_page][seg] = new_display_page;
		}

		number_pixel+=8;
		display_pixel+=8;
		number_page = number_pixel/8;
		display_page = display_pixel/8;
	}

	// Write what is left of last page of number on the beginning of next display page
	if (number_pixel < NUMBER_PAGE_COUNT*8){
		for(int seg=0;seg<NUMBER_WIDTH;seg++){
			int pixels_to_end = NUMBER_PAGE_COUNT*8 - number_pixel;
			int pixels_from_last_page = 8 - pixels_to_end;

			uint8_t old_left = (dest[display_page][seg] >> pixels_to_end) << pixels_to_end;
			uint8_t new_right = icon[number_page][seg] >> pixels_from_last_page;

			dest[display_page][seg] = old_left | new_right;
		}
	}
}



void display_meters(uint32_t meters){
	uint8_t meters_display[MAX_PAGE_COUNT][NUMBER_WIDTH] = {0};

	if(meters == 0){	
		write_number_icon(meters_display,numbers[0],0,3);
		display_numbers(meters_display);
		return;
	}

	if(meters >= 1000){	
		write_number_icon(meters_display,greater_than,42,3);
		write_number_icon(meters_display,numbers[1],21,3);
		write_number_icon(meters_display,km,0,3);
		display_numbers(meters_display);
		return;
	}

	
	int curr_pixel = 0;
	while(meters != 0){
		uint32_t digit = meters%10;

		write_number_icon(meters_display,numbers[digit],curr_pixel,3);

		meters/=10;
		curr_pixel+=21;
	}
	display_numbers(meters_display);
}

void update_dir_display(struct dir_data* data){
	if(data->dir == TURN_SHARP_LEFT){
		display_turn(turn_sharp_left);
	}else if(data->dir == UTURN_RIGHT){
		display_turn(uturn_right);
	}else if(data->dir == TURN_SLIGHT_RIGHT){	
		display_turn(turn_slight_right);
	}else if(data->dir == MERGE){
		display_turn(merge);
	}else if(data->dir == ROUNDABOUT_LEFT){
		display_turn(roundabout_left);
	}else if(data->dir == ROUNDABOUT_RIGHT){	
		display_turn(roundabout_right);
	}else if(data->dir == UTURN_LEFT){
		display_turn(uturn_left);
	}else if(data->dir == TURN_LEFT){
		display_turn(turn_left);
	}else if(data->dir == RAMP_RIGHT){	
		display_turn(ramp_right);
	}else if(data->dir == TURN_RIGHT){
		display_turn(turn_right);
	}else if(data->dir == FORK_RIGHT){
		display_turn(fork_right);
	}else if(data->dir == FERRY){
		display_turn(ferry);
	}else if(data->dir == STRAIGHT){
		display_turn(straight);
	}else if(data->dir == TURN_LEFT){	
		display_turn(turn_left);
	}else if(data->dir == FORK_LEFT){		
		display_turn(fork_left);
	}else if(data->dir == FERRY_TRAIN){
		display_turn(ferry_train);
	}else if(data->dir == TURN_SHARP_RIGHT){	
		display_turn(turn_sharp_right);
	}else if(data->dir == RAMP_LEFT){
		display_turn(ramp_left);
	}else if(data->dir == NO_DIR){
		clear_display();
	}else{
		ESP_LOGW(tag,"Invalid direction in characteristic value attribute");
	}

	char meters_str[10];
	sprintf(meters_str,"%dm",data->meters);
	ESP_LOGI(tag,"Displaying %d meters",data->meters);
}

int currMeters=700;
void dir_disp_task(void *pvParameter){
    while(1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);

		struct dir_data currDir;
		get_dir_status(&currDir);

		display_meters(currDir.meters);

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

	xTaskCreate(&dir_disp_task, "display_dir_on_oled", 4098, NULL, 5, NULL);
}
