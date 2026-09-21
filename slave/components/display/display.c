#include "display.h"
#include "amoled_driver.h"

static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv;
static SemaphoreHandle_t xGuiSemaphore  = NULL;
static lv_obj_t *chart                  = NULL;
static lv_chart_series_t *ser           = NULL;

static void lvgl_task(void *arg);

static void lvgl_flush_cb(lv_disp_drv_t *drv,
                        const lv_area_t *area,
                        lv_color_t *color_map)
{
    uint32_t width  = area->x2 - area->x1 + 1;
    uint32_t height = area->y2 - area->y1 + 1;

    amoled_set_window(
        area->x1,
        area->y1,
        area->x2,
        area->y2
    );

    amoled_push_buffer(
        (uint16_t *)color_map,
        width * height
    );

    lv_disp_flush_ready(drv);
}

static void lvgl_tick_cb(void *arg)
{
    lv_tick_inc(2);
}

void display_init_ui(void)
{
    xGuiSemaphore = xSemaphoreCreateMutex();
    assert(xGuiSemaphore != NULL);

    printf("Initializing AMOLED...\n");
    display_init();

    printf("Initializing LVGL...\n");
    lv_init();

    const size_t buffer_pixels = 536 * 240;

    lv_color_t *buf1 = heap_caps_malloc(
        buffer_pixels * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM
    );

    lv_color_t *buf2 = heap_caps_malloc(
        buffer_pixels * sizeof(lv_color_t),
        MALLOC_CAP_SPIRAM
    );

    assert(buf1);
    assert(buf2);

    lv_disp_draw_buf_init(
        &draw_buf,
        buf1,
        buf2,
        buffer_pixels
    );

    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res    = 536;
    disp_drv.ver_res    = 240;
    disp_drv.flush_cb   = lvgl_flush_cb;
    disp_drv.draw_buf   = &draw_buf;

    lv_disp_drv_register(&disp_drv);

    const esp_timer_create_args_t tick_args = {
        .callback   = lvgl_tick_cb,
        .name       = "lvgl_tick",
    };

    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 2000));

    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    chart = lv_chart_create(lv_scr_act());
    lv_obj_set_size(chart, 500, 200);
    lv_obj_center(chart);
    
    lv_chart_set_point_count(chart, 64);
    
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3300);

    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

    xSemaphoreGive(xGuiSemaphore);

    xTaskCreate(
        lvgl_task,
        "lvgl",
        4096,
        NULL,
        5,
        NULL
    );
}

void display_update_wave(const display_packet_t *data)
{
    if (xGuiSemaphore != NULL && xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        
        for (int i = 0; i < 64; i++)
            lv_chart_set_value_by_id(chart, ser, i, (lv_coord_t)data->voltages[i]);
        
        lv_chart_refresh(chart);
        
        xSemaphoreGive(xGuiSemaphore);
    }
}

static void lvgl_task(void *arg)
{
    while (1) {
        if (xGuiSemaphore != NULL && xSemaphoreTake(xGuiSemaphore, portMAX_DELAY) == pdTRUE) {
            uint32_t delay_ms = lv_timer_handler();
            xSemaphoreGive(xGuiSemaphore);

            if (delay_ms < 1)
                delay_ms = 1;
            else if (delay_ms > 10)
                delay_ms = 10;

            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
}