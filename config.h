
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CLIENT_WINDOW_CAP 256
#define MOD Mod4Mask


//APPLICATION
#define CMD_BROWSER     "firefox &"
#define CMD_TERMINAL    "st &"
#define CMD_APPLAUNCHER "rofi -show drun &"
#define CMD_VOLUME_UP   "pactl set-sink-volume @DEFAULT_SINK@ +5%"
#define CMD_VOLUME_DOWN "pactl set-sink-volume @DEFAULT_SINK@ -5%"
#define CMD_VOLUME_MUTE "pactl set-sink-mute   @DEFAULT_SINK@ toggle"

//WINDOW RELATED 
#define BORDER_WIDTH  2
#define FBORDER_COLOR 0x8aadf4
#define UBORDER_COLOR 0x2E3440
#define BG_COLOR      0xffffff


//TODO: Implement This feature
//WINDOW NAVIGATOR KEY (VIM LIKE)
#define NAVIGATE_UP  XK_K
#define NAVIGATE_DOWN XK_J
//WINDOW NAVIGATOR END



//KEYBINDING
#define OPEN_TERMINAL   XK_Return
#define OPEN_BROWSER    XK_W
#define OPEN_LAUNCHER   XK_D
#define CLOSE_WINDOW    XK_Q //ok 
#define KILL_WM         XK_C
#define FULL_SCREEN     XK_F //ok
#define MAKE_TILE    XK_T //ok
#define MINI_APP  XK_U
#define MAX_MINI_APP XK_I
// #define NAVIGATE_MAX_APP XK_L


//WORKSPACE NAVIGATOR
#define CHANGE_WORKSPACE XK_N //ok
#define CHANGE_WORKSPACE_BACK XK_B //ok
#define CHANGE_ACTIVE_WORKSPACE XK_P
#define SWAP_WINDOW XK_A //ok
#define SWAP_UP_DOWN XK_Z //ok
#define MOVE_WINDOW_NEXT XK_H



//Volumet Related
#define VOLUME_UP   XK_Up
#define VOLUME_DOWN XK_Down
#define VOLUME_MUTE XK_M


#define DISPLAY_HEIGHT  745
#define DISPLAY_WIDTH   1366
#define WORKSPACE 4



//DECORATION RELATED
#define FONT                                    "JetBrains Mono Nerd Font:size=10:style=bold"
#define FONT_SIZE                               15
#define FONT_COLOR                              "#f9b424"
#define DECORATION_FONT_COLOR                   "#ff0000"

#define MAXIMIZE_ICON                ""
#define MAXIMIZE_ICON_COLOR          "#f9b424"

#define TITLE_BAR_BG_COLOR  0x302d35

#define CLOSE_ICON ""
#define CLOSE_ICON_COLOR "#fb5246"

#define ICON_SIZE 20
#define TITLE_BAR_HEIGHT 20
#define BUTTON_GAPS 20


#define TEST XK_Y
