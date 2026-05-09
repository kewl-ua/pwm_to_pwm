#include <Arduino.h>
#include "driver/ledc.h"
#include "esp_timer.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Пины PWM ──────────────────────────────────────
#define PIN_PWM_IN   2
#define PIN_PWM_OUT  3

// ── Пины I2C (SSD1306) ────────────────────────────
#define PIN_SDA      7
#define PIN_SCL      6

// ── Диапазоны ─────────────────────────────────────
#define IN_MIN   1000
#define IN_MAX   2000
#define OUT_MIN  500 
#define OUT_MAX  2500

#define SANITY_MIN  (OUT_MIN - 100)   // 400
#define SANITY_MAX  (OUT_MAX + 100)   // 2600

// ── LEDC ──────────────────────────────────────────
#define LEDC_TIMER      LEDC_TIMER_0
#define LEDC_CHANNEL    LEDC_CHANNEL_0
#define LEDC_RES        LEDC_TIMER_14_BIT
#define SERVO_HZ        50

// ── Дисплей ───────────────────────────────────────
#define SCREEN_W    128
#define SCREEN_H     64
#define OLED_ADDR  0x3C

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ── Состояние входного PWM ────────────────────────
volatile uint32_t pwm_rise_us = 0;
volatile uint16_t pwm_in_us   = 1500;

// ── Прерывание ────────────────────────────────────
void IRAM_ATTR onPwmEdge() {
    uint32_t now = (uint32_t)esp_timer_get_time();
    if (digitalRead(PIN_PWM_IN)) {
        pwm_rise_us = now;
    } else {
        uint32_t width = now - pwm_rise_us;

        if (width >= SANITY_MIN && width <= SANITY_MAX) {
            pwm_in_us = (uint16_t)width;
        }
    }
}

// ── Конвертация диапазонов ────────────────────────
static inline uint16_t convert(uint16_t in) {
    in = constrain(in, IN_MIN, IN_MAX);
    return (uint16_t)(OUT_MIN + (uint32_t)(in - IN_MIN) * (OUT_MAX - OUT_MIN) / (IN_MAX - IN_MIN));
}

// ── us → duty LEDC ────────────────────────────────
static inline uint32_t us_to_duty(uint16_t us) {
    return (uint32_t)us * 16383 / 20000;
}

// ── Отрисовка дисплея ─────────────────────────────
void updateDisplay(uint16_t in_us, uint16_t out_us) {
    display.clearDisplay();

    // ── Заголовок ─────────────────────────────────
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("PWM CONVERTER");
    display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

    // ── Входное значение ──────────────────────────
    display.setTextSize(1);
    display.setCursor(0, 13);
    display.print("IN:");
    display.setTextSize(2);
    display.setCursor(24, 11);
    display.print(in_us);
    display.setTextSize(1);
    display.setCursor(90, 18);
    display.print("us");

    // ── Разделитель ───────────────────────────────
    display.drawLine(0, 33, 127, 33, SSD1306_WHITE);

    // ── Выходное значение (крупнее) ───────────────
    display.setTextSize(1);
    display.setCursor(0, 37);
    display.print("OUT:");
    display.setTextSize(2);
    display.setCursor(30, 35);
    display.print(out_us);
    display.setTextSize(1);
    display.setCursor(102, 42);
    display.print("us");

    // ── Прогресс-бар выхода ───────────────────────
    // Маппируем out_us [750..2500] на ширину [0..124]
    int bar_w = map(out_us, OUT_MIN, OUT_MAX, 0, 124);
    display.drawRect(2, 55, 124, 8, SSD1306_WHITE);
    display.fillRect(2, 55, bar_w, 8, SSD1306_WHITE);

    display.display();
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // I2C на кастомных пинах
    Wire.begin(PIN_SDA, PIN_SCL);

    // Инициализация дисплея
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("SSD1306 not found!");
        while (true) delay(100);
    }
    display.clearDisplay();
    display.display();

    // LEDC таймер
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_RES,
        .timer_num       = LEDC_TIMER,
        .freq_hz         = SERVO_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    // LEDC канал
    ledc_channel_config_t channel_cfg = {
        .gpio_num   = PIN_PWM_OUT,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = LEDC_CHANNEL,
        .timer_sel  = LEDC_TIMER,
        .duty       = us_to_duty(1500),
        .hpoint     = 0,
    };
    ledc_channel_config(&channel_cfg);

    // Прерывание входного PWM
    pinMode(PIN_PWM_IN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIN_PWM_IN), onPwmEdge, CHANGE);

    Serial.println("Ready");
}

void loop() {
    uint16_t in  = pwm_in_us;
    uint16_t out = convert(in);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, us_to_duty(out));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);

    updateDisplay(in, out);

    Serial.printf("in: %4d us  ->  out: %4d us\n", in, out);
    delay(50);  // ~20 fps на дисплее, серво успевает
}