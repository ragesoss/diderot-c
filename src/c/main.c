#include <pebble.h>
#include "main.h"

#define ARTICLE 0
#define IS_NEW 1
#define DISTANCE 2

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_nearest_article_layer;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

//////////////////////
// Build the window //
//////////////////////
static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(25, 20), bounds.size.w, 50));

  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));

  // Create nearest_article Layer
  s_nearest_article_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(75, 70), bounds.size.w, 50));

  // Style the text
  text_layer_set_background_color(s_nearest_article_layer, GColorClear);
  text_layer_set_text_color(s_nearest_article_layer, GColorBlack);
  text_layer_set_font(s_nearest_article_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text_alignment(s_nearest_article_layer, GTextAlignmentCenter);
  text_layer_set_text(s_nearest_article_layer, "...");
  
  layer_add_child(window_layer, text_layer_get_layer(s_nearest_article_layer));
}

static void main_window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_nearest_article_layer);
}


///////////////////////
// message callbacks //
///////////////////////
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "ohai!");
  // Store incoming information
  static char article_buffer[256];
  //static char new_buffer[8];
  //static char distance_buffer[32];
  //static char direction_buffer[8];

  Tuple *article_tuple = dict_find(iterator, ARTICLE);
  snprintf(article_buffer, sizeof(article_buffer), "%s", article_tuple->value->cstring);
  text_layer_set_text(s_nearest_article_layer, article_buffer);

  if(!article_tuple) {
    APP_LOG(APP_LOG_LEVEL_INFO, "nope!");
  }
  APP_LOG(APP_LOG_LEVEL_INFO, "kthxbai!");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

/////////////////////////////
// Main setup and teardown //
/////////////////////////////
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register message callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

