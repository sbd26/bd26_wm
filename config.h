typedef struct {
  char app_name[20];
  int t_workspace;
  bool is_floating;
  bool will_focused;
}Rule;

const char *startup_commands[] = {"killall dunst &",
                                  "setxkbmap us &",
                                  "nitrogen --restore &",
                                  "dunst &",
                                  "picom &",
                                  "polybar &"};

const Rule rules[] = {
  {.app_name = "Thunar", .t_workspace = 2, .is_floating = true, .will_focused = false},
  {.app_name = "firefox", .t_workspace = 0, .will_focused = true},
};

#define MOD Mod4Mask

#define BAR_SIZE 23
//APPLICATION
#define CMD_BROWSER     "firefox &"
#define CMD_TERMINAL    "st &"
#define CMD_APPLAUNCHER "rofi -show drun &"
#define CMD_VOLUME_UP   "pactl set-sink-volume @DEFAULT_SINK@ +5%"
#define CMD_VOLUME_DOWN "pactl set-sink-volume @DEFAULT_SINK@ -5%"
#define CMD_VOLUME_MUTE "pactl set-sink-mute   @DEFAULT_SINK@ toggle"
#define CMD_SCREENSHOT  "xfce4-screenshooter &"

//WINDOW RELATED 
#define BORDER_WIDTH  2
#define FBORDER_COLOR 0x99d1db
#define UBORDER_COLOR 0x2E3440


#define CHANGE_TILE_STYLE XK_S

//WINDOW NAVIGATOR KEY (VIM LIKE)
#define NAVIGATE_UP  XK_K
#define NAVIGATE_DOWN XK_J
//WINDOW NAVIGATOR END


//TILING RELATED
#define INCREASE_DECREASE_MASTER_SIZE XK_L


//KEYBINDING
#define OPEN_TERMINAL   XK_Return
#define OPEN_BROWSER    XK_W
#define OPEN_LAUNCHER   XK_D
#define CLOSE_WINDOW    XK_Q //ok 
#define SCREENSHOT_KEY  XK_Print
#define KILL_WM         XK_C
#define FULL_SCREEN     XK_F //ok
#define MAKE_TILE    XK_T //ok
#define MINI_APP  XK_U //ok
#define MAX_MINI_APP XK_I //ok
// #define NAVIGATE_MAX_APP XK_L


//WORKSPACE NAVIGATOR
#define CHANGE_WORKSPACE XK_N //ok
#define CHANGE_WORKSPACE_BACK XK_B //ok
#define CHANGE_ACTIVE_WORKSPACE XK_P
#define SWAP_WINDOW XK_A //ok
#define SWAP_UP_DOWN XK_Z //ok
#define MOVE_WINDOW_NEXT XK_H //ok
#define MOVE_WINDOW_PREV XK_G //ok


#define INCREASE_GAPS XK_Plus
#define DECREASE_GAPS XK_Minus


//Volumet Related
#define VOLUME_UP   XK_Up
#define VOLUME_DOWN XK_Down
#define VOLUME_MUTE XK_M


#define WORKSPACE 4



//DECORATION RELATED


#define FONT                                    "FantasqueSansM Nerd Font:size=13:style=bold"
#define FONT_SIZE                               15
#define FONT_COLOR                              "#f9b424"
#define DECORATION_FONT_COLOR                   "#302d35"

#define MAXIMIZE_ICON                ""
#define MAXIMIZE_ICON_COLOR          "#f9b424"

#define TITLE_BAR_BG_COLOR  0x302d35

#define CLOSE_ICON ""
#define CLOSE_ICON_COLOR "#fb5246"

#define ICON_SIZE 20
#define TITLE_BAR_HEIGHT 20
#define BUTTON_GAPS 20

#define XA_ATOM 4

#define BG_COLOR      0xffffff
