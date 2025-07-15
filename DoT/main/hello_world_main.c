#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include <string.h>
#include "driver/uart.h"
#include "esp_err.h"
#include <stdio.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#define WIFI_SSID "cheongjukgwan2"
#define WIFI_PASS "Djedsmhspw2015!"

static const char *TAG = "WIFI";

void wifi_init_sta(void){
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS, 
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();

    ESP_LOGE(TAG, "와파 연결중..");
}

#define three_GPIO 13
#define five_GPIO 15
#define six_GPIO 16 // 16번 
#define seven_GPIO 17 // 17번 핀
#define eight_GPIO 18 // 18번 핀
#define nine_GPIO 19

#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_MAX_DEGREE 180
#define BUF_SIZE 1024

//us는 마이크로초 단위의 펄스폭을 의미함
static uint32_t servo_us_to_duty(uint32_t us) { // us는 값이 쉽게 커질 수 있기에 넉넉한 32
    return (us * (1 << 15)) / 20000; // 시간을 듀티 사이클 값으로 변환함
}

void rotate_servo_360(uint32_t channel, int direction) { // 채널과 방향의 인자값을 받음
    uint32_t us;

    if (direction == 0) { // 정지
        us = 1500; 
    } else if (direction > 0) { // 우회전
        us = 1700; 
    } else { // 좌회전
        us = 1300; 
    }

    uint32_t duty = servo_us_to_duty(us); // 듀티 사이클 값으로 반환
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty); // 지정 채널의 듀티 사이클 설정
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel); // 지정 채널에 업로드
}

void servo_motor(void) {

    ledc_timer_config_t ledc_timer = { // Ledc는 PWM을 활용하는 모든 하드웨어
        .duty_resolution = LEDC_TIMER_15_BIT, // 듀티 해상도 비트 수 (얼마나 세세히 조정하는가?)
        .freq_hz = 50, // 주파수 설정 (1초에 몇번의 신호가 반복되는가)
        .speed_mode = LEDC_LOW_SPEED_MODE, // 저속 모드
        .timer_num = LEDC_TIMER_0 // 반복 주시 생성
    };

    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
    
    ledc_channel_config_t ledc_channel13 = {
        .channel    = LEDC_CHANNEL_5, // 채널 번호, 각 채널은 독립적인 PWM신호를 생성함 (중복된 채널을 있을 수 없음)
        .duty       = 0, // 초기 듀티 사이클 (0 . . . 100)
        .gpio_num   = three_GPIO, // PWM신호를 출력할 PIN번호
        .speed_mode = LEDC_LOW_SPEED_MODE, // 저속 모드
        .hpoint     = 0, // 처음 신호가 시작되는 시점(offset)을 뜻함, 만약 값이 5라면 5틱 후 HIGH
        .timer_sel  = LEDC_TIMER_0 // 한 번 꺼졌다 켜졌다를 반복할 주기를 만듦
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel13));

    ledc_channel_config_t ledc_channel15 = {
        .channel    = LEDC_CHANNEL_0, // 채널 번호, 각 채널은 독립적인 PWM신호를 생성함 (중복된 채널을 있을 수 없음)
        .duty       = 0, // 초기 듀티 사이클 (0 . . . 100)
        .gpio_num   = five_GPIO, // PWM신호를 출력할 PIN번호
        .speed_mode = LEDC_LOW_SPEED_MODE, // 저속 모드
        .hpoint     = 0, // 처음 신호가 시작되는 시점(offset)을 뜻함, 만약 값이 5라면 5틱 후 HIGH
        .timer_sel  = LEDC_TIMER_0 // 한 번 꺼졌다 켜졌다를 반복할 주기를 만듦
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel15));

    ledc_channel_config_t ledc_channel16 = {
        .channel    = LEDC_CHANNEL_1, // 채널 번호, 각 채널은 독립적인 PWM신호를 생성함 (중복된 채널을 있을 수 없음)
        .duty       = 0, // 초기 듀티 사이클 (0 . . . 100)
        .gpio_num   = six_GPIO, // PWM신호를 출력할 PIN번호
        .speed_mode = LEDC_LOW_SPEED_MODE, // 저속 모드
        .hpoint     = 0, // 처음 신호가 시작되는 시점(offset)을 뜻함, 만약 값이 5라면 5틱 후 HIGH
        .timer_sel  = LEDC_TIMER_0 // 한 번 꺼졌다 켜졌다를 반복할 주기를 만듦
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel16));

    ledc_channel_config_t ledc_channel17 = {
        .channel    = LEDC_CHANNEL_2, // 채널 번호, 각 채널은 독립적인 PWM신호를 생성함
        .duty       = 0, // 초기 듀티 사이클 (0 . . . 100)
        .gpio_num   = seven_GPIO, // PWM신호를 출력할 PIN번호
        .speed_mode = LEDC_LOW_SPEED_MODE, // 저속 모드
        .hpoint     = 0, // 처음 신호가 시작되는 시점(offset)을 뜻함, 만약 값이 5라면 5틱 후 HIGH
        .timer_sel  = LEDC_TIMER_0 // 한 번 꺼졌다 켜졌다를 반복할 주기를 만듦
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel17)); // 구초체 값을 넘겨 값 설정

    ledc_channel_config_t ledc_channel18 = {
        .channel    = LEDC_CHANNEL_3, // 채널 번호, 각 채널은 독립적인 PWM신호를 생성함 (중복된 채널을 있을 수 없음)
        .duty       = 0, // 초기 듀티 사이클 (0 . . . 100)
        .gpio_num   = eight_GPIO, // PWM신호를 출력할 PIN번호
        .speed_mode = LEDC_LOW_SPEED_MODE, // 저속 모드
        .hpoint     = 0, // 처음 신호가 시작되는 시점(offset)을 뜻함, 만약 값이 5라면 5틱 후 HIGH
        .timer_sel  = LEDC_TIMER_0 // 한 번 꺼졌다 켜졌다를 반복할 주기를 만듦
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel18));

    ledc_channel_config_t ledc_channel19 = {
            .channel    = LEDC_CHANNEL_4, // 채널 번호, 각 채널은 독립적인 PWM신호를 생성함 (중복된 채널을 있을 수 없음)
            .duty       = 0, // 초기 듀티 사이클 (0 . . . 100)
            .gpio_num   = nine_GPIO, // PWM신호를 출력할 PIN번호
            .speed_mode = LEDC_LOW_SPEED_MODE, // 저속 모드
            .hpoint     = 0, // 처음 신호가 시작되는 시점(offset)을 뜻함, 만약 값이 5라면 5틱 후 HIGH
            .timer_sel  = LEDC_TIMER_0 // 한 번 꺼졌다 켜졌다를 반복할 주기를 만듦
        };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel19));

    // UART 기본 설정
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM_0, &uart_config); // 기본 통신 설정
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0); // UART 드라이버 설치 / 설정

    uint8_t uart_buff[BUF_SIZE + 1] = { 0 }; // 버퍼 크기 설정
    ledc_channel_t channel_List[6] = { // 채널 리스트
        LEDC_CHANNEL_5,
        LEDC_CHANNEL_0,
        LEDC_CHANNEL_1,
        LEDC_CHANNEL_2,
        LEDC_CHANNEL_3,
        LEDC_CHANNEL_4
    };

    char *dots = "100000"; // l 

    while (1) {
        for (int i = 0; i < 6; i++) {
            if (dots[i] == '1') {
                rotate_servo_360(channel_List[i], 1);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));

        for (int i = 0; i < 6; i++) {
            if (dots[i] == '1') {
                rotate_servo_360(channel_List[i], -1);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    } 
    vTaskDelay(pdMS_TO_TICKS(1000));
    // 버퍼 정리
    memset(uart_buff, 0, sizeof(uart_buff)); // 메모리 초기화 (+공부)
    uart_flush_input(UART_NUM_0);  // 다음 입력에 영향 안 주도록 초기화 (버퍼 clear)
}

void app_main(void){
    nvs_flash();
    wifi_init_sta();
    servo_motor();
}