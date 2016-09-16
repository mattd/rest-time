#include <pebble.h>

#define PERSIST_WORK_INTERVAL 10
#define PERSIST_REST_INTERVAL 20
#define PERSIST_WARNING_VIBRATION 30
#define PERSIST_OVERRUNABLE 40

#define DEFAULT_WORK_INTERVAL 1500
#define DEFAULT_REST_INTERVAL 120
#define DEFAULT_WARNING_VIBRATION false
#define DEFAULT_OVERRUNABLE false

#define NUM_MENU_SECTIONS 1
#define NUM_MENU_ITEMS 4

#define MAX_WORK_INTERVAL 3600
#define WORK_INTERVAL_INCREMENT 300

#define MAX_REST_INTERVAL 600
#define REST_INTERVAL_INCREMENT 60

#define WARNING_VIBRATION_TIME 9

#define TIME_STR_LENGTH 6

static int WORK_INTERVAL;
static int STARTING_WORK_INTERVAL;

static int REST_INTERVAL;
static int STARTING_REST_INTERVAL;

static bool WARNING_VIBRATION;

static bool OVERRUNABLE;

static Window *s_main_window;
static Window *s_menu_window;

static TextLayer *s_clock_layer;
static TextLayer *s_countdown_layer;
static TextLayer *s_paused_indicator_layer;

static SimpleMenuLayer *s_simple_menu_layer;
static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuItem s_menu_items[NUM_MENU_ITEMS];

static bool s_in_rest_mode = false;
static bool s_countdown_paused = true;

static int s_countdown_seconds;

static void init_settings() {
   WORK_INTERVAL = (
       persist_exists(PERSIST_WORK_INTERVAL) ?
           persist_read_int(PERSIST_WORK_INTERVAL) :
           DEFAULT_WORK_INTERVAL
   );
   REST_INTERVAL = (
       persist_exists(PERSIST_REST_INTERVAL) ?
           persist_read_int(PERSIST_REST_INTERVAL) :
           DEFAULT_REST_INTERVAL
   );
   WARNING_VIBRATION = (
       persist_exists(PERSIST_WARNING_VIBRATION) ?
           persist_read_bool(PERSIST_WARNING_VIBRATION) :
           DEFAULT_WARNING_VIBRATION
   );
   OVERRUNABLE = (
       persist_exists(PERSIST_OVERRUNABLE) ?
           persist_read_bool(PERSIST_OVERRUNABLE) :
           DEFAULT_OVERRUNABLE
   );
}

static char* format_countdown_time(int countdown_time, char* str) {
    countdown_time = abs(countdown_time);
    int seconds = countdown_time % 60;
    int minutes = countdown_time / 60;

    snprintf(str, TIME_STR_LENGTH, "%01d:%02d", minutes, seconds);

    return str;
}

static void update_work_interval(int index, void *context) {
    static char countdown_str[TIME_STR_LENGTH];

    if (WORK_INTERVAL == MAX_WORK_INTERVAL) {
        WORK_INTERVAL = WORK_INTERVAL_INCREMENT;
    } else {
        WORK_INTERVAL += WORK_INTERVAL_INCREMENT;
    }

    s_menu_items[0].subtitle = format_countdown_time(
        WORK_INTERVAL,
        countdown_str
    );

    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void update_rest_interval(int index, void *context) {
    static char countdown_str[TIME_STR_LENGTH];

    if (REST_INTERVAL == MAX_REST_INTERVAL) {
        REST_INTERVAL = REST_INTERVAL_INCREMENT;
    } else {
        REST_INTERVAL += REST_INTERVAL_INCREMENT;
    }

    s_menu_items[1].subtitle = format_countdown_time(
        REST_INTERVAL,
        countdown_str
    );

    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void update_warning_vibration(int index, void *context) {
    WARNING_VIBRATION = !WARNING_VIBRATION;

    s_menu_items[2].subtitle = WARNING_VIBRATION ? "On" : "Off";

    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void update_overrun(int index, void *context) {
    OVERRUNABLE = !OVERRUNABLE;

    s_menu_items[3].subtitle = OVERRUNABLE ? "On" : "Off";

    layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void build_menu() {
    static char work_countdown_str[TIME_STR_LENGTH];
    static char rest_countdown_str[TIME_STR_LENGTH];

    s_menu_items[0] = (SimpleMenuItem) {
        .title = "Work Interval",
        .subtitle = format_countdown_time(WORK_INTERVAL, work_countdown_str),
        .callback = update_work_interval
    };
    s_menu_items[1] = (SimpleMenuItem) {
        .title = "Rest Interval",
        .subtitle = format_countdown_time(REST_INTERVAL, rest_countdown_str),
        .callback = update_rest_interval
    };
    s_menu_items[2] = (SimpleMenuItem) {
        .title = "Warning Vibe",
        .subtitle = WARNING_VIBRATION ? "On" : "Off",
        .callback = update_warning_vibration
    };
    s_menu_items[3] = (SimpleMenuItem) {
        .title = "Overrun",
        .subtitle = OVERRUNABLE ? "On" : "Off",
        .callback = update_overrun
    };
    s_menu_sections[0] = (SimpleMenuSection) {
        .title = "Settings",
        .num_items = NUM_MENU_ITEMS,
        .items = s_menu_items
    };
}

static void set_colors() {
    if (s_in_rest_mode) {
        window_set_background_color(s_main_window, GColorWhite);

        text_layer_set_background_color(s_clock_layer, GColorWhite);
        text_layer_set_text_color(s_clock_layer, GColorBlack);

        text_layer_set_background_color(s_countdown_layer, GColorWhite);
        text_layer_set_text_color(s_countdown_layer, GColorBlack);

        text_layer_set_background_color(s_paused_indicator_layer, GColorWhite);
        text_layer_set_text_color(s_paused_indicator_layer, GColorBlack);
    } else {
        window_set_background_color(s_main_window, GColorBlack);

        text_layer_set_background_color(s_clock_layer, GColorBlack);
        text_layer_set_text_color(s_clock_layer, GColorWhite);

        text_layer_set_background_color(s_countdown_layer, GColorBlack);
        text_layer_set_text_color(s_countdown_layer, GColorWhite);

        text_layer_set_background_color(s_paused_indicator_layer, GColorBlack);
        text_layer_set_text_color(s_paused_indicator_layer, GColorWhite);
    }
}

static void update_clock_time() {
    time_t temp = time(NULL);

    struct tm *tick_time = localtime(&temp);

    static char *buffer = "00:00";

    if (clock_is_24h_style() == true) {
        strftime(buffer, TIME_STR_LENGTH, "%H:%M", tick_time);
    } else {
        strftime(buffer, TIME_STR_LENGTH, "%I:%M", tick_time);
    }

    text_layer_set_text(s_clock_layer, buffer);
}

static void update_countdown_layer() {
    static char countdown_str[TIME_STR_LENGTH];    
    text_layer_set_text(
        s_countdown_layer,
        format_countdown_time(s_countdown_seconds, countdown_str)
    );
}

static void update_pause_indicator_layer() {
    if (s_countdown_paused) {
        int interval = s_in_rest_mode ? REST_INTERVAL : WORK_INTERVAL;
        if (s_countdown_seconds == interval) {
            text_layer_set_text(s_paused_indicator_layer, "Ready");
        } else {
            text_layer_set_text(s_paused_indicator_layer, "Paused");
        }
    } else {
        if (s_countdown_seconds < 0) {
            text_layer_set_text(s_paused_indicator_layer, "Overrun");
        } else {
            text_layer_set_text(s_paused_indicator_layer, "");
        }      
    }
}

static void start_mode(bool is_rest_mode) {
    s_in_rest_mode = is_rest_mode;
    set_colors();
    if (is_rest_mode) {
        s_countdown_seconds = REST_INTERVAL;
        vibes_double_pulse();
    } else {
        s_countdown_seconds = WORK_INTERVAL;
        vibes_short_pulse();
    }
    s_countdown_paused = false;
}

static void time_tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    if (units_changed | MINUTE_UNIT) {
        update_clock_time();
    }
    
    if (s_countdown_paused) {
        return;
    }

    if (s_countdown_seconds == 0 && !OVERRUNABLE) {
        start_mode(!s_in_rest_mode);
    } else {

        --s_countdown_seconds; 
        update_countdown_layer();
        if (s_countdown_seconds == -1) {
            update_pause_indicator_layer();
        }

        if (s_countdown_seconds <= 0 && s_countdown_seconds % 60 == 0) {
            vibes_short_pulse();
        }

        if (
            WARNING_VIBRATION &&
            s_countdown_seconds == WARNING_VIBRATION_TIME &&
            !s_in_rest_mode
        ) {
            vibes_double_pulse();
        }
    }
}

static void persist_config() {
    persist_write_int(PERSIST_WORK_INTERVAL, WORK_INTERVAL);
    persist_write_int(PERSIST_REST_INTERVAL, REST_INTERVAL);
    persist_write_bool(PERSIST_WARNING_VIBRATION, WARNING_VIBRATION);
    persist_write_bool(PERSIST_OVERRUNABLE, OVERRUNABLE);
}

static void main_window_load(Window *window) {
    static char countdown_str[TIME_STR_LENGTH];

    s_clock_layer = text_layer_create(GRect(5, 85, 144, 50));
    s_countdown_layer = text_layer_create(GRect(5, 50, 144, 42));
    s_paused_indicator_layer = text_layer_create(GRect(5, 34, 60, 16));

    text_layer_set_font(
        s_clock_layer,
        fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD)
    );
    text_layer_set_font(
        s_countdown_layer,
        fonts_get_system_font(FONT_KEY_BITHAM_34_MEDIUM_NUMBERS)
    );

    set_colors();

    layer_add_child(
        window_get_root_layer(window),
        text_layer_get_layer(s_clock_layer)
    );
    layer_add_child(
        window_get_root_layer(window),
        text_layer_get_layer(s_countdown_layer)
    );
    layer_add_child(
        window_get_root_layer(window),
        text_layer_get_layer(s_paused_indicator_layer)
    );

    update_clock_time();

    text_layer_set_text(
        s_countdown_layer,
        format_countdown_time(s_countdown_seconds, countdown_str)
    );
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_clock_layer);
    text_layer_destroy(s_countdown_layer);
    text_layer_destroy(s_paused_indicator_layer);
}

static void menu_window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_frame(window_layer);

    STARTING_WORK_INTERVAL = WORK_INTERVAL;
    STARTING_REST_INTERVAL = REST_INTERVAL;

    s_simple_menu_layer = simple_menu_layer_create(
        bounds,
        window,
        s_menu_sections,
        NUM_MENU_SECTIONS,
        NULL
    );

    layer_add_child(
        window_layer,
        simple_menu_layer_get_layer(s_simple_menu_layer)
    );
}

static void menu_window_unload(Window *window) {
    persist_config();

    if (
        STARTING_REST_INTERVAL != REST_INTERVAL ||
        STARTING_WORK_INTERVAL != WORK_INTERVAL
    ) {
        s_countdown_paused = true;
        s_countdown_seconds = WORK_INTERVAL;
        s_in_rest_mode = false;

        update_countdown_layer();
        set_colors();
    }
    simple_menu_layer_destroy(s_simple_menu_layer);
}

static void select_single_click_handler (ClickRecognizerRef recognizer,
                                         void *context) {
    s_countdown_paused = !s_countdown_paused;
    update_pause_indicator_layer();
}

static void up_single_click_handler (ClickRecognizerRef recognizer,
                                     void *context) {
    start_mode(true);
    update_countdown_layer();
    update_pause_indicator_layer();
}

static void down_single_click_handler (ClickRecognizerRef recognizer,
                                       void *context) {
    start_mode(false);
    update_countdown_layer();
    update_pause_indicator_layer();
}

static void select_multi_click_handler (ClickRecognizerRef recognizer,
                                        void *context) {
    window_stack_push(s_menu_window, true);
}

static void click_config_provider (Window *window) {
    window_single_click_subscribe(
        BUTTON_ID_SELECT,
        select_single_click_handler
    );
    window_single_click_subscribe(
        BUTTON_ID_UP,
        up_single_click_handler
    );
    window_single_click_subscribe(
        BUTTON_ID_DOWN,
        down_single_click_handler
    );
    window_multi_click_subscribe(
        BUTTON_ID_SELECT,
        2,
        10,
        0,
        true,
        select_multi_click_handler
    );
}

static void init() {
    // Initialize core.
    init_settings();
    build_menu();

    // Setup clock updates.
    s_countdown_seconds = WORK_INTERVAL;
    tick_timer_service_subscribe(SECOND_UNIT, time_tick_handler);

    // Create and configure main window.
    s_main_window = window_create();

    #ifdef PBL_SDK_2
    window_set_fullscreen(s_main_window, true);
    #endif

    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });

    window_set_click_config_provider(
        s_main_window,
        (ClickConfigProvider) click_config_provider
    );

    // Create and configure menu window.
    s_menu_window = window_create();

    window_set_window_handlers(s_menu_window, (WindowHandlers) {
        .load = menu_window_load,
        .unload = menu_window_unload
    });

    // Kick things off.
    window_stack_push(s_main_window, true);
}

static void deinit() {
    window_destroy(s_main_window);
    window_destroy(s_menu_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
