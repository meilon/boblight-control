#include "pebble.h"
#include "mini-printf.h"

#define NUM_MENU_SECTIONS 2
#define NUM_MENU_ICONS 2
#define NUM_FIRST_MENU_ITEMS 2
#define NUM_SECOND_MENU_ITEMS 3

static Window *window;

// This is a menu layer
// You have more control than with a simple menu layer
static MenuLayer *menu_layer;

// Menu items can optionally have an icon drawn with them
static GBitmap *menu_icons[NUM_MENU_ICONS];

static int status = 0;
static char *status_str[2] = {"Lights are off", "Lights are on"};
static int preset = 0;
char *preset_str[]= {"None", "Rot", "Grün", "Warmweiß hell", "Warmweiß dunkel"};
int preset_cnt = ARRAY_LENGTH(preset_str);

static int red = 10;
static int green = 10;
static int blue = 10;
static char strRed[100] = "";
static char strGreen[100] = "";
static char strBlue[100] = "";

enum AppSyncKey {
  AMKEY_INIT = 0x0,
  AMKEY_STATUS = 0x1,
  AMKEY_RED = 0x2,
  AMKEY_GREEN = 0x3,
  AMKEY_BLUE = 0x4,
  AMKEY_PRESETS = 0x5,
  AMKEY_PRESET = 0x6,
};

 AppSync sync;
 uint8_t sync_buffer[256];

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Dict Message Sync Error: %d", dict_error);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case AMKEY_STATUS:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Status : %d", new_tuple->value->uint8);
      status = new_tuple->value->uint8;
      break;
    case AMKEY_RED:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Red    : %d", new_tuple->value->uint8);
      red = new_tuple->value->uint8;
      mini_snprintf(strRed, 15, "Currently: %d", red);
      break;
    case AMKEY_GREEN:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Green  : %d", new_tuple->value->uint8);
      green = new_tuple->value->uint8;
      mini_snprintf(strGreen, 15, "Currently: %d", green);
      break;
    case AMKEY_BLUE:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Blue   : %d", new_tuple->value->uint8);
      blue = new_tuple->value->uint8;
      mini_snprintf(strBlue, 15, "Currently: %d", blue);
      break;
    case AMKEY_PRESET:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Preset : %d", new_tuple->value->uint8);
      preset = new_tuple->value->uint8;
      break;
    case AMKEY_PRESETS:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Presets: %s", new_tuple->value->cstring);
      break;
  }
  layer_mark_dirty(menu_layer_get_layer(menu_layer));
}

// You can draw arbitrary things in a menu item such as a background
static GBitmap *menu_background;

// A callback is used to specify the amount of sections of menu items
// With this, you can dynamically add and remove sections
static uint16_t menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
  return NUM_MENU_SECTIONS;
}

// Each section has a number of items;  we use a callback to specify this
// You can also dynamically add and remove items using this
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  switch (section_index) {
    case 0:
      return NUM_FIRST_MENU_ITEMS;

    case 1:
      return NUM_SECOND_MENU_ITEMS;

    default:
      return 0;
  }
}

// A callback is used to specify the height of the section header
static int16_t menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  // This is a define provided in pebble.h that you may use for the default height
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

// Here we draw what each header is
static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
  // Determine which section we're working with
  switch (section_index) {
    case 0:
      // Draw title text in the section header
      menu_cell_basic_header_draw(ctx, cell_layer, "Master Control");
      break;
    
    case 1:
      menu_cell_basic_header_draw(ctx, cell_layer, "RGB Control");
      break;
  }
}

// This is the menu item draw callback where you specify what each item should look like
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
  // Determine which section we're going to draw in
  switch (cell_index->section) {
    case 0:
      // Use the row to specify which item we'll draw
      switch (cell_index->row) {
        case 0:
          // This is a basic menu item with a title and subtitle
          menu_cell_basic_draw(ctx, cell_layer, "Lights ON/OFF", status_str[status], menu_icons[status]);
          break;
        
        case 1:
          // There is title draw for something more simple than a basic menu item
          menu_cell_basic_draw(ctx, cell_layer, "Presets", preset_str[preset], NULL);
          break;
      }
	    break;
	case 1:	  
	  switch (cell_index->row) {
        case 0:
          // This is a basic menu icon with a cycling icon
          menu_cell_basic_draw(ctx, cell_layer, "Red", strRed, NULL);
          break;
        case 1:
          // This is a basic menu icon with a cycling icon
          menu_cell_basic_draw(ctx, cell_layer, "Green", strGreen, NULL);
          break;
        case 2:
          // This is a basic menu icon with a cycling icon
          menu_cell_basic_draw(ctx, cell_layer, "Blue", strBlue, NULL);
          break;

        case 3:
          // Here we use the graphics context to draw something different
          // In this case, we show a strip of a watchface's background
          graphics_draw_bitmap_in_rect(ctx, menu_background,
              (GRect){ .origin = GPointZero, .size = layer_get_frame((Layer*) cell_layer).size });
          break;
      }
      break;
  }
}

void send_to_phone(uint32_t key, uint8_t value) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Got send_to_phone request for key %d with val %d", (uint8_t)key, value);
  Tuplet new_tuples[] = {
    TupletInteger(key, value),
  };
  app_sync_set(&sync, new_tuples, ARRAY_LENGTH(new_tuples));
}

// Here we capture when a user selects a menu item
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
  // Use the row to specify which item will receive the select action
  switch (cell_index->section) {
    case 0:
      switch (cell_index->row) {
        // This is the menu item with the cycling icon
        case 0:
          if (status == 0) {
            status = 1;
          } else {
            status = 0;
          }
          // After changing the icon, mark the layer to have it updated
          layer_mark_dirty(menu_layer_get_layer(menu_layer));
          send_to_phone(AMKEY_STATUS, (uint8_t) status);
          break;
        case 1:
          preset = (preset + 1) % preset_cnt;
          layer_mark_dirty(menu_layer_get_layer(menu_layer));
          send_to_phone(AMKEY_PRESET, (uint8_t) preset);
      }
      break;
    case 1:
      switch (cell_index->row) {
        case 0:
          red = (red + 10) % 260;
          mini_snprintf(strRed, 15, "Currently: %d", red);
          layer_mark_dirty(menu_layer_get_layer(menu_layer));
          send_to_phone(AMKEY_RED, (uint8_t) red);
          break;
        case 1:
          green = (green + 10) % 260;
          mini_snprintf(strGreen, 15, "Currently: %d", green);
          layer_mark_dirty(menu_layer_get_layer(menu_layer));
          send_to_phone(AMKEY_GREEN, (uint8_t) green);
          break;
        case 2:
          blue = (blue + 10) % 260;
          mini_snprintf(strBlue, 15, "Currently: %d", blue);
          layer_mark_dirty(menu_layer_get_layer(menu_layer));
          send_to_phone(AMKEY_BLUE, (uint8_t) blue);
          break;
      }
      break;
  }  
}

static void send_cmd(void) {
  Tuplet value = TupletInteger(1, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

// This initializes the menu upon window load
void window_load(Window *window) {
  // Here we load the bitmap assets
  // resource_init_current_app must be called before all asset loading
  int num_menu_icons = 0;
  menu_icons[num_menu_icons++] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON_LIGHTS_OFF);
  menu_icons[num_menu_icons++] = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON_LIGHTS_ON);

  // And also load the background
  menu_background = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND_BRAINS);

  // Now we prepare to initialize the menu layer
  // We need the bounds to specify the menu layer's viewport size
  // In this case, it'll be the same as the window's
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  
  mini_snprintf(strRed, 15, "Currently: %d", red);
  mini_snprintf(strBlue, 15, "Currently: %d", blue);
  mini_snprintf(strGreen, 15, "Currently: %d", green);

  // Create the menu layer
  menu_layer = menu_layer_create(bounds);

  // Set all the callbacks for the menu layer
  menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
    .get_num_sections = menu_get_num_sections_callback,
    .get_num_rows = menu_get_num_rows_callback,
    .get_header_height = menu_get_header_height_callback,
    .draw_header = menu_draw_header_callback,
    .draw_row = menu_draw_row_callback,
    .select_click = menu_select_callback,
  });

  // Bind the menu layer's click config provider to the window for interactivity
  menu_layer_set_click_config_onto_window(menu_layer, window);

  // Add it to the window for display
  layer_add_child(window_layer, menu_layer_get_layer(menu_layer));
  
  
  Tuplet initial_values[] = {
    TupletInteger(AMKEY_STATUS, (uint8_t) 0),
    TupletInteger(AMKEY_RED, (uint8_t) 0),
    TupletInteger(AMKEY_GREEN, (uint8_t) 0),
    TupletInteger(AMKEY_BLUE, (uint8_t) 0),
    TupletInteger(AMKEY_PRESET, (uint8_t) 0),
    TupletCString(AMKEY_PRESETS, "")
  };
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer),
               initial_values, ARRAY_LENGTH(initial_values),
               sync_tuple_changed_callback, sync_error_callback, NULL);
  
  send_cmd();
}

void window_unload(Window *window) {
  // Destroy the menu layer
  menu_layer_destroy(menu_layer);

  // Cleanup the menu icons
  for (int i = 0; i < NUM_MENU_ICONS; i++) {
    gbitmap_destroy(menu_icons[i]);
  }

  // And cleanup the background
  gbitmap_destroy(menu_background);
  app_sync_deinit(&sync);
}

static void send_init(void *data) {
  send_to_phone(AMKEY_INIT, 1);
}

int main(void) {
  window = window_create();

  // Setup the window handlers
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  window_stack_push(window, true /* Animated */);
  
  app_message_open(64, 64);
  
  app_timer_register(150, send_init, NULL);

  app_event_loop();

  window_destroy(window);
}
