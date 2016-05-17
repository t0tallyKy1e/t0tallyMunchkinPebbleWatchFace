/* Created by t0tallKy1e
 * 05/15/2016
 * main file
*/

#include <pebble.h>
static Window *s_main_window;
static TextLayer *s_time_layer;
static GFont custom_font;

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
static GBitmapSequence *s_sequence;
static GBitmap *s_charging_bitmap;

static void handle_bluetooth(bool connected){
    if(connected){
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ALIEN_BT_CONNECTED);
    }//close if
    else{
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ALIEN_MUNCHKIN_COLOR_FACE);
    }//close else
}//close handle_bluetooth

static void timer_handler(void *context){
    uint32_t next_delay;

    if(gbitmap_sequence_update_bitmap_next_frame(s_sequence, s_charging_bitmap, &next_delay)){
        //put image in layer
        bitmap_layer_set_bitmap(s_background_layer, s_charging_bitmap);

        layer_mark_dirty(bitmap_layer_get_layer(s_background_layer));

        app_timer_register(next_delay, timer_handler, NULL);
    }//close if
}//close timer_handler

static void handle_battery(BatteryChargeState charge_state){
    if(charge_state.is_charging){
        //create animation
        s_sequence = gbitmap_sequence_create_with_resource(RESOURCE_ID_TK_CHARGING);
    
        //create blank gbitmap with animation size
        GSize frame_size = gbitmap_sequence_get_bitmap_size(s_sequence);
        s_charging_bitmap = gbitmap_create_blank(frame_size, GBitmapFormat8Bit);
        
        //initiate
        uint32_t first_delay_ms = 10;
        app_timer_register(first_delay_ms, timer_handler, NULL);
    }//close if
    else if(charge_state.charge_percent == 100){
        //create charged image
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ALIEN_MUNCHKIN_CHARGED);

        //put image in layer
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    }//close else if
    else if(charge_state.charge_percent < 20){
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ALIEN_DYING_BATT);
        
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    }//close else if
    else{
        //put image in layer
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    }//close else
}//close handle_battery

static void update_time(){
    //get tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    //write current minutes/hours into buffer
    static char s_buffer[8];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

    //display time
    text_layer_set_text(s_time_layer, s_buffer);
}//close update_time

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  update_time();
}//close tick_handler

static void main_window_load(Window *window){
  //window info
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  //load font
  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_TOTALLYKYLE_40));
  
  //create text layer with bounds
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58,130), bounds.size.w - 5, 50));
  
  //make layout look like a watchface
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_font(s_time_layer, custom_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentRight);
  
  handle_bluetooth(connection_service_peek_pebble_app_connection());
  battery_state_service_subscribe(handle_battery);
  
  //create bitmap layer to display image
  s_background_layer = bitmap_layer_create(bounds);
  
  connection_service_subscribe((ConnectionHandlers){
    .pebble_app_connection_handler = handle_bluetooth
  });
  
  //add as child layer of window's root layer
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  handle_battery(battery_state_service_peek());
}//close main_window_load

static void main_window_unload(Window *window){
  battery_state_service_unsubscribe();
  connection_service_unsubscribe();
  
  //destroy text layer
  text_layer_destroy(s_time_layer);
  fonts_unload_custom_font(custom_font);
  
  //destroy background image
  gbitmap_destroy(s_background_bitmap);
  
  //destroy charging animation
  gbitmap_sequence_destroy(s_sequence);
  gbitmap_destroy(s_charging_bitmap);
  
  //destroy background image layer
  bitmap_layer_destroy(s_background_layer);
}//close main_window_unload

static void init(){
  //create main window
  s_main_window = window_create();
  
  //handlers to manage elements of window
  window_set_window_handlers(s_main_window, (WindowHandlers){
    .load = main_window_load,
    .unload = main_window_unload
  });//close window_set_window_handlers
  
  //show window on watch
  window_stack_push(s_main_window, true);
  
  //register tick timer service
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  update_time();
}//close init

static void deinit(){
  //destroy window
  window_destroy(s_main_window);
}//close deinit

int main(void){
  init();
  app_event_loop();
  deinit();
  
  return 0;
}//close main