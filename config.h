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
#define BORDER_WIDTH  3
#define FBORDER_COLOR 0xff0000
// #define ABORDER_COLOR 0xff0000
#define DESKTOP_COUNT 9
#define BG_COLOR      0xffffff

//KEYBINDING
#define OPEN_TERMINAL   XK_Return
#define OPEN_BROWSER    XK_W
#define OPEN_LAUNCHER   XK_D
#define CLOSE_WINDOW    XK_Q
#define KILL_WM         XK_C
#define FULL_SCREEN     XK_F
#define CYCLE_WINDOW    XK_Tab
#define TITLE_AGAIN     XK_T


#define DISPLAY_HEIGHT  768
#define DISPLAY_WIDTH   1366
