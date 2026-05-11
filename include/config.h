#pragma once

// ── Пины PWM ──────────────────────────────────────
#define PIN_PWM_IN   2
#define PIN_PWM_OUT  3

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
#ifdef USE_DISPLAY
#define PIN_SDA     7
#define PIN_SCL     6
#define SCREEN_W    128
#define SCREEN_H    64
#define OLED_ADDR   0x3C
#endif
