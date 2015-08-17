/*
 * otpwatch.c
 *
 * Let's see what this'll do.
 */

#include "pebble.h"
#include "config.h"
#include "pebble_totp.h"

enum {
	TEXT_LAYER_TOPDATE,
	TEXT_LAYER_BATT,
	TEXT_LAYER_TOKEN,
	TEXT_LAYER_DATE,
	TEXT_LAYER_TIME,

	TEXT_LAYER_LAST
};

enum {
	IMG_LAYER_BT,
	IMG_LAYER_BATTERY,
	IMG_LAYER_CHARGING,

	IMG_LAYER_LAST
};

enum {
	IMG_BLUETOOTH,
	IMG_CHARGING,
	IMG_BATTERY,

	IMG_LAST
};

enum {
	LAYER_BATT_PERC,
	LAYER_LINE,

	LAYER_LAST
};

#ifndef COLOR_SCHEME_INVERT
# define FG_COLOR GColorBlack
# define BG_COLOR GColorWhite
#else
# define FG_COLOR GColorWhite
# define BG_COLOR GColorBlack
#endif

static Window *s_main_window;
static TextLayer *s_text_layer[TEXT_LAYER_LAST];
static GBitmap *s_image[IMG_LAST];
static BitmapLayer *s_img_layer[IMG_LAYER_LAST];
static Layer *s_layer[LAYER_LAST];
static pebble_totp token;

static BatteryChargeState s_batt_state;


static void
batt_perc_layer_update_callback(Layer *layer, GContext *ctx)
{
	GRect rect = layer_get_bounds(layer);
	rect.size.w *= s_batt_state.charge_percent;
	rect.size.w /= 100;

	graphics_context_set_fill_color(ctx, FG_COLOR);
	graphics_fill_rect(ctx, rect, 0, GCornerNone);
}

static void
line_layer_update_callback(Layer *layer, GContext *ctx)
{
	graphics_context_set_fill_color(ctx, FG_COLOR);
	graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
}

static void
handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
	/* Need to be static because they're used by the system later. */
	static char s_topdate_text[] = "YYYY-MM-DD";
	static char s_token_text[] = "123 456";
	static char s_date_text[] = "Xxxxxxxxx 00";
	static char s_time_text[] = "00:00";
	const char *code = pebble_totp_get_code(&token, mktime(tick_time));

	strncpy(s_token_text, code, 3);
	strncpy(s_token_text + 4, code + 3, 3);

	strftime(s_topdate_text, sizeof(s_topdate_text), "%F", tick_time);
	strftime(s_date_text, sizeof(s_date_text), "%B %e", tick_time);

	if (clock_is_24h_style())
		strftime(s_time_text, sizeof(s_time_text), "%R", tick_time);
	else
		strftime(s_time_text, sizeof(s_time_text), "%I:%M", tick_time);

	text_layer_set_text(s_text_layer[TEXT_LAYER_TOPDATE], s_topdate_text);
	text_layer_set_text(s_text_layer[TEXT_LAYER_TOKEN], s_token_text);
	text_layer_set_text(s_text_layer[TEXT_LAYER_DATE], s_date_text);
	text_layer_set_text(s_text_layer[TEXT_LAYER_TIME], s_time_text);
}

static void
handle_battery(BatteryChargeState state)
{
	static char s_batt_text[] = "100%";

	s_batt_state = state;

	layer_set_hidden(bitmap_layer_get_layer(s_img_layer[IMG_LAYER_CHARGING]),
			 !state.is_charging);

	snprintf(s_batt_text, sizeof(s_batt_text), "%d%%", state.charge_percent);
	text_layer_set_text(s_text_layer[TEXT_LAYER_BATT], s_batt_text);
}

static void
update_bt_layer(bool connected)
{
	layer_set_hidden(bitmap_layer_get_layer(s_img_layer[IMG_LAYER_BT]),
			 !connected);
}

static void
handle_bt(bool connected)
{
	static const uint32_t const segments[] = { 300, 200, 300, 400, 500 };
	VibePattern pat = {
		.durations = segments,
		.num_segments = ARRAY_LENGTH(segments),
	};

	update_bt_layer(connected);
	if (connected)
		vibes_short_pulse();
	else
		vibes_enqueue_custom_pattern(pat);
}

static void
main_window_load(Window * window)
{
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	int w = bounds.size.w;

#define CREATE_TEXT_LAYER(ind, x, y, w, h, font, align)			\
	s_text_layer[ind] = text_layer_create(GRect(x, y, w, h));	\
	text_layer_set_font(s_text_layer[ind],				\
			    fonts_get_system_font(font));		\
			    text_layer_set_text_alignment(s_text_layer[ind], \
							  GTextAlignment ## align); \
			    text_layer_set_text_color(s_text_layer[ind], FG_COLOR); \
			    text_layer_set_background_color(s_text_layer[ind], \
							    GColorClear); \
			    layer_add_child(window_layer,		\
					    text_layer_get_layer(s_text_layer[ind]))

#define CREATE_IMG_LAYER(NAME, IMGNAME, x, y, w, h)			\
	s_img_layer[IMG_LAYER_ ## NAME] = bitmap_layer_create(GRect(x, y, w, h)); \
	bitmap_layer_set_bitmap(s_img_layer[IMG_LAYER_ ## NAME],	\
				s_image[IMG_ ## IMGNAME]);		\
	layer_add_child(window_layer,					\
			bitmap_layer_get_layer(s_img_layer[IMG_LAYER_ ## NAME]))

#define CREATE_LAYER(NAME, name, x, y, w, h)				\
	s_layer[LAYER_ ## NAME] = layer_create(GRect(x, y, w, h));	\
	layer_set_update_proc(s_layer[LAYER_ ## NAME],			\
			      name ## _layer_update_callback);		\
	layer_add_child(window_layer, s_layer[LAYER_ ## NAME])


	CREATE_TEXT_LAYER(TEXT_LAYER_TOPDATE, 0, 0, w, 14, FONT_KEY_GOTHIC_14, Right);
	CREATE_TEXT_LAYER(TEXT_LAYER_BATT, 28, 0, w - 28, 18, FONT_KEY_GOTHIC_14, Left);
	CREATE_TEXT_LAYER(TEXT_LAYER_TOKEN, 0, 22, w, 18, FONT_KEY_GOTHIC_18_BOLD, Center);
	CREATE_TEXT_LAYER(TEXT_LAYER_DATE, 0, 68, w, 100, FONT_KEY_ROBOTO_CONDENSED_21, Center);
	CREATE_TEXT_LAYER(TEXT_LAYER_TIME, 0, 92, w, 76, FONT_KEY_ROBOTO_BOLD_SUBSET_49, Center);

	/* Battery composition has it's own order */
	CREATE_IMG_LAYER(BATTERY, BATTERY, 0, 2, 25, 12);
	CREATE_LAYER(BATT_PERC, batt_perc, 2, 4, 20, 8);
	CREATE_IMG_LAYER(CHARGING, CHARGING, 9, 2, 6, 12);

	CREATE_IMG_LAYER(BT, BLUETOOTH, (w / 2) - 4, 3, 7, 11);
	CREATE_LAYER(LINE, line, 8, 97, w - 16, 2);

#undef CREATE_TEXT_LAYER
#undef CREATE_IMG_LAYER
#undef CREATE_LAYER
}

static void
main_window_unload(Window * window)
{
	int i = 0;

	for (i = 0; i < TEXT_LAYER_LAST; i++)
		text_layer_destroy(s_text_layer[i]);
	for (i = 0; i < IMG_LAYER_LAST; i++)
		bitmap_layer_destroy(s_img_layer[i]);
	for (i = 0; i < LAYER_LAST; i++)
		layer_destroy(s_layer[i]);
	for (i = 0; i < IMG_LAST; i++)
		gbitmap_destroy(s_image[i]);
}

static void
init()
{
	const unsigned char key[] = TOTP_SECRET;
	WindowHandlers win_handlers = {
		.load = main_window_load,
		.unload = main_window_unload,
	};

#define LOAD_IMG(name)							\
	s_image[IMG_ ## name] = gbitmap_create_with_resource(RESOURCE_ID_ ## name)

	LOAD_IMG(BLUETOOTH);
	LOAD_IMG(CHARGING);
	LOAD_IMG(BATTERY);

#undef LOAD_IMG

	pebble_totp_init(&token, key, sizeof(key), TOTP_INTERVAL);

	s_main_window = window_create();
	window_set_background_color(s_main_window, BG_COLOR);
	window_set_window_handlers(s_main_window, win_handlers);
	window_stack_push(s_main_window, true);

	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	handle_minute_tick(tm, MINUTE_UNIT);

	battery_state_service_subscribe(handle_battery);
	handle_battery(battery_state_service_peek());

	bluetooth_connection_service_subscribe(handle_bt);
	update_bt_layer(bluetooth_connection_service_peek());
}

static void
deinit()
{
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	bluetooth_connection_service_unsubscribe();
	window_destroy(s_main_window);
}

int
main()
{
	init();
	app_event_loop();
	deinit();
}
