#include <limits.h>
#include "hlc_tft_display/config.h"
#include "hlc_tft_display/hlc_tft_display.h"

painter_device_t lcd;
painter_device_t lcd_surface;

#define STAT_TYPE unsigned
#define MAX_STAT_TYPE UINT_MAX

static STAT_TYPE thirst;
static STAT_TYPE fatigue;
static STAT_TYPE hunger;

struct color_t {
    uint8_t hue, sat, val;
};

static const struct color_t thirst_color = {187, 255, 255};
static const struct color_t fatigue_color = {56, 255, 255};
static const struct color_t hunger_color = {255, 255, 255};

static const uint32_t min = 60 * 1000;

/// This function is ran on bootup of the keyboard.
bool module_post_init_user(void) {
    thirst = MAX_STAT_TYPE;
    fatigue = MAX_STAT_TYPE;
    hunger = MAX_STAT_TYPE;
    return false;
}

void drain_stat(STAT_TYPE* const stat, const uint32_t elapsed_time, const uint32_t total_drain_time) {
    const STAT_TYPE drained = ((double)elapsed_time / total_drain_time) * MAX_STAT_TYPE;

    *stat = drained > *stat ? 0 : *stat - drained;
}

bool critically_low(const STAT_TYPE val) {
    return val < (0.2 * MAX_STAT_TYPE);
}

static unsigned Nbar = 8;

unsigned bar_slot_height(void) {
    return LCD_HEIGHT / Nbar;
}

unsigned bar_height(void) {
    return 0.8 * bar_slot_height();
}

bool draw_bar(const unsigned slot, const STAT_TYPE val, const struct color_t color) {
    if (slot >= Nbar) { return false; }

    const uint16_t left = 0;
    const uint16_t right = LCD_WIDTH * ((double)val / MAX_STAT_TYPE);

    const uint16_t free_vert_space = bar_slot_height() - bar_height();
    const uint16_t top = slot * bar_slot_height() + free_vert_space / 2;
    const uint16_t bottom = top + bar_height();

    return qp_rect(lcd_surface,
                   left,
                   top,
                   right,
                   bottom,
                   color.hue,
                   color.sat,
                   color.val,
                   true);
}

bool draw_bar_outline(const unsigned slot, const struct color_t color) {
    if (slot >= Nbar) { return false; }

    const uint16_t left = 0;
    const uint16_t right = LCD_WIDTH;

    const uint16_t top = slot * bar_slot_height();
    const uint16_t bottom = top + bar_slot_height();

    return qp_rect(lcd_surface,
                   left,
                   top,
                   right,
                   bottom,
                   color.hue,
                   color.sat,
                   color.val,
                   false);
}

/// This function runs after every matrix scan.
bool display_module_housekeeping_task_user(bool second_display) {
    // Don't do anything with a potential second screen.
    if (second_display) { return true; }

    static uint32_t last_draw = 0;
    const uint32_t refresh_interval = 500;
    const uint32_t elapsed_time = timer_elapsed32(last_draw);

    static bool blinker = true;

    if (elapsed_time > refresh_interval) { // 33) { // Throttle to 30fps
        last_draw = timer_read32();

        blinker = !blinker;

        draw_bar(0, thirst, thirst_color);
        draw_bar(1, fatigue, fatigue_color);
        draw_bar(2, hunger, hunger_color);

        if (blinker) {
            if (critically_low(thirst)) { draw_bar_outline(0, thirst_color); }
            if (critically_low(fatigue)) { draw_bar_outline(1, fatigue_color); }
            if (critically_low(hunger)) { draw_bar_outline(2, hunger_color); }
        }

        drain_stat(&thirst, elapsed_time, 0.3 * min);
        drain_stat(&fatigue, elapsed_time, 0.4 * min);
        drain_stat(&hunger, elapsed_time, 0.2 * min);

        // Move surface to lcd, this actually writes the content to the physical display.
        qp_surface_draw(lcd_surface, lcd, 0, 0, 0);
        qp_clear(lcd_surface);
    }

    return false;
}
