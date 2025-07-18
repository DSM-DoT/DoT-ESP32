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
#include "esp_websocket_client.h"
#include "cJSON.h"

#define WIFI_SSID "3314" // 변경 가능
#define WIFI_PASS "20071001"

#define three_GPIO 13
#define five_GPIO 15
#define six_GPIO 16
#define seven_GPIO 17
#define eight_GPIO 18
#define nine_GPIO 19

#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_MAX_DEGREE 180
#define BUF_SIZE 1024

char binary[128];
bool websocket_start = false;

static const char *TAG_WIFI = "WIFI";
static const char *TAG_WSS = "WSS";

void websocket_app_start(void);
void servo_motor(void);
void channel(void);
void wifi_init_sta(void);

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG_WIFI, "와이파이 연결 재시도 중...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        if (!websocket_start) {
            websocket_app_start();
            websocket_start = true;
        }
    }
}

void wifi_init_sta(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "Wi-Fi 초기화 완료, 연결 시도 중...");
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;

    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG_WSS, "WebSocket 연결 성공");
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG_WSS, "WebSocket 연결 종료");
            break;
        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI(TAG_WSS, "메시지 수신 [%.*s]", data->data_len, (char *)data->data_ptr);

            cJSON *root = cJSON_ParseWithLength(data->data_ptr, data->data_len);
            if (root) {
                cJSON *val = cJSON_GetObjectItem(root, "message");
                if (val && cJSON_IsString(val)) {
                    strncpy(binary, val->valuestring, sizeof(binary));
                    binary[sizeof(binary) - 1] = '\0';
                    ESP_LOGI(TAG_WSS, "수신된 값: %s", binary);
                    servo_motor();
                } else {
                    ESP_LOGW(TAG_WSS, "\"value\" 필드가 없거나 문자열이 아님");
                }
                cJSON_Delete(root);
            } else {
                ESP_LOGW(TAG_WSS, "JSON 파싱 실패");
            }
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG_WSS, "WebSocket 에러 발생");
            break;
        default:
            break;
    }
}

void websocket_app_start(void) {
    extern const uint8_t _binary_ca_cert_pem_start[];
    
    esp_websocket_client_config_t websocket_cfg = {
        .uri = "wss://zooming-contentment-production.up.railway.app",
        .disable_auto_reconnect = false,
        .reconnect_timeout_ms = 5000,
        .cert_pem = (const char *)_binary_ca_cert_pem_start,
        .use_global_ca_store = false,
        // .skip_cert_common_name_check = true,
        .transport = WEBSOCKET_TRANSPORT_OVER_SSL,
    };

    esp_websocket_client_handle_t client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, ESP_EVENT_ANY_ID, websocket_event_handler, (void *)client);
    esp_websocket_client_start(client);
}


static uint32_t servo_us_to_duty(uint32_t us) {
    return (us * (1 << 15)) / 20000;
}

void rotate_servo_360(uint32_t channel, int direction) {
    uint32_t us;

    if (direction == 0) {
        us = 1500; // 정지 신호 (중립)
    } else if (direction > 0) {
        us = 1900; // 정방향 회전
    } else {
        us = 1100; // 역방향 회전
    }

    uint32_t duty = servo_us_to_duty(us);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel);
}

void channel(void) {
    // PWM 타이머 설정
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_15_BIT,
        .freq_hz = 50,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 6개 PWM 채널 설정
    struct {
        ledc_channel_t channel;
        int gpio_num;
    } pwm_pins[] = {
        {LEDC_CHANNEL_5, three_GPIO},
        {LEDC_CHANNEL_0, five_GPIO},
        {LEDC_CHANNEL_1, six_GPIO},
        {LEDC_CHANNEL_2, seven_GPIO},
        {LEDC_CHANNEL_3, eight_GPIO},
        {LEDC_CHANNEL_4, nine_GPIO},
    };

    for (int i = 0; i < 6; i++) {
        ledc_channel_config_t ledc_channel = {
            .channel = pwm_pins[i].channel,
            .duty = 0,
            .gpio_num = pwm_pins[i].gpio_num,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint = 0,
            .timer_sel = LEDC_TIMER_0,
        };
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }
}

void servo_motor(void) {
    ledc_channel_t channel_List[6] = {
        LEDC_CHANNEL_5,
        LEDC_CHANNEL_0,
        LEDC_CHANNEL_1,
        LEDC_CHANNEL_2,
        LEDC_CHANNEL_3,
        LEDC_CHANNEL_4
    };

    char *dots = binary;
    uint32_t idx = 0;

    // binary 문자열을 6개씩 끊어서 처리
    while (dots[idx] != '\0') {
        // 정방향 회전
        for (uint32_t i = idx; i < idx + 6 && dots[i] != '\0'; i++) {
            if (dots[i] == '1') {
                rotate_servo_360(channel_List[i % 6], 1);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));

        // 역방향 회전
        for (uint32_t i = idx; i < idx + 6 && dots[i] != '\0'; i++) {
            if (dots[i] == '1') {
                rotate_servo_360(channel_List[i % 6], -1);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(250));

        // 정지
        for (uint32_t i = idx; i < idx + 6 && dots[i] != '\0'; i++) {
            if (dots[i] == '1') {
                rotate_servo_360(channel_List[i % 6], 0);
            }
        }

        idx += 6;
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    // UART 초기화 (필요시)
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, BUF_SIZE * 2, 0, 0, NULL, 0);

    wifi_init_sta();
    channel();
}
