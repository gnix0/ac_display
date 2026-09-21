#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "display.h"
#include <assert.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_heap_caps.h"
#include "esp_timer.h"

#include "lvgl.h"

typedef struct {
    uint32_t voltages[64];
} __attribute__((packed)) display_packet_t;

void display_init_ui(void);
void display_update_wave(const display_packet_t *data);

#endif // display.h