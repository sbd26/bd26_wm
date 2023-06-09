#include <stdlib.h>


typedef enum {
  WINDOW_LAYOUT_TILED = 0,
  WINDOW_LAYOUT_TILED_VERTICAL = 1,
} WindowLayout;



typedef struct {
    XftFont* font;
    XftColor color;
    XftDraw* draw;
}FontStruct;

typedef struct{
  Window close_button;
  Window maximize_button;
  Window title_bar;
  FontStruct close_button_font, maximize_button_font, title_bar_font;
}MAC_DECOR;

typedef enum {
  MINI_STATE = 0,
  NORMAL_STATE = 1
} CurrentState;

typedef struct{
  Window win;
}Bar;

typedef struct {
  float x, y;
} Vec2;

typedef struct {
  Window win;
  Window frame;
  bool fullscreen;
  Vec2 fullscreen_revert_size;
  Vec2 fullscreen_revert_pos;
  bool is_floating;
  bool was_focused;
  MAC_DECOR decoration;
} Client;


typedef struct {
  Display *display;
  Window root;
  bool running;
  uint8_t window_gap;
  
  WindowLayout current_layout[WORKSPACE];
  Client client_windows[WORKSPACE][CLIENT_WINDOW_CAP];
  uint32_t clients_count[WORKSPACE];
  Vec2 cursor_start_pos, cursor_start_frame_pos, cursor_start_frame_size;
  bool swap_done[WORKSPACE];
  Bar bar;
  int screen;
  CurrentState currentstate[WORKSPACE];
}bd26;

