#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/Xcomposite.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include "config.h"


const char *startup_commands[] = {
  "setxkbmap us &",
  "nitrogen --restore &",
  "picom &"
};


typedef enum {
  WINDOW_LAYOUT_TILED = 0,
  WINDOW_LAYOUT_TILED_VERTICAL = 1
} WindowLayout;

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
} Client;


typedef struct {
  Display *display;
  Window root;
  bool running;
  uint8_t window_gap;
  
  WindowLayout current_layout;
  Client client_windows[CLIENT_WINDOW_CAP];
  uint32_t clients_count;
  Vec2 cursor_start_pos, cursor_start_frame_pos, cursor_start_frame_size;
}bd26;

static bool wm_detected = false;
static bd26 wm;

static void handle_create_notify(XCreateWindowEvent e) { (void)e;}
static void handle_configure_notify(XConfigureEvent e) {(void)e;}
static int handle_wm_detected(Display *display, XErrorEvent *e){(void)display; wm_detected = ((int32_t)e->error_code == BadAccess); return 0;}
static void handle_reparent_notify(XReparentEvent e){(void)e;}
static void handle_destroy_notify(XDestroyWindowEvent e) {(void)e;}
static void handle_map_notify(XMapEvent e) {(void)e;}
static void handle_button_release(XButtonEvent e) {(void)e;}
static void handle_key_release(XKeyEvent e) {(void)e;}
static int handle_x_error(Display *display, XErrorEvent *e) {    (void)display;
    char err_msg[1024];
    XGetErrorText(display, e->error_code, err_msg, sizeof(err_msg));
    printf("X Error:\n\tRequest: %i\n\tError Code: %i - %s\n\tResource ID: %i\n", 
           e->request_code, e->error_code, err_msg, (int)e->resourceid); return 0;
}
static void handle_configure_request(XConfigureRequestEvent e);
static void handle_map_request(XMapRequestEvent e); //okay
static void handle_unmap_notify(XUnmapEvent e);
static void handle_key_press(XKeyEvent e);
static void handle_button_press(XButtonEvent e);
static void handle_motion_notify(XMotionEvent e);
//-----Other Functions start
static void set_fullscreen(Window win);
static void unset_fullscreen(Window win);
static void window_frame(Window win);
static void window_unframe(Window win);
static Window get_frame_window(Window win);
static int32_t get_client_index(Window win);
static void cycle_window(Window win);
static void move_client(Client *client, Vec2 pos);
static void resize_client(Client *client, Vec2 sz);
static void grab_global_key();
static void grab_window_key(Window win);
static void establish_window_layout(bool restore_back);
//------other Functions end

//tiling related function

void establish_window_layout(bool restore_back){
  Client * tmp_clients[CLIENT_WINDOW_CAP];
  uint32_t clients_count = 0;

  for (uint32_t i = 0; i < wm.clients_count; i++){
    if (!wm.client_windows[i].is_floating || restore_back){
      tmp_clients[clients_count++] = &wm.client_windows[i];
    }
  }
  if (restore_back){
  for (uint32_t i = 0; i < clients_count; i++){
    tmp_clients[i]->is_floating = false;
  }
  }

  if (clients_count == 0)return;
  if (wm.current_layout == WINDOW_LAYOUT_TILED){
    Client * rooT = tmp_clients[0];

    if (clients_count == 1){
      set_fullscreen(rooT -> frame);
      return;
    }

    resize_client(rooT, (Vec2){.x = (float)DISPLAY_WIDTH / 2, .y = DISPLAY_HEIGHT - 20});
    move_client(rooT, (Vec2){.x = 5, .y = 10});
    rooT -> fullscreen = false;
    float y_cordintae = 0;

    for (uint32_t i = 1; i < clients_count; i++){
      resize_client(tmp_clients[i], (Vec2){.x = ((float)DISPLAY_WIDTH / 2) - 22, .y = ((float)DISPLAY_HEIGHT / (clients_count - 1) - 20)});
      move_client(tmp_clients[i], (Vec2){.x = ((float)DISPLAY_WIDTH / 2 + 15), .y = y_cordintae + 10 });
      y_cordintae += ((float) DISPLAY_HEIGHT / (clients_count - 1));
    }
  }
}




void establish_window_layout_bak(){
  int32_t master_index = -1;
  uint32_t clients_on_monitor = 0;
  bool found_master = false;

  for (uint32_t i = 0; i < wm.clients_count; i++){
      if (!found_master){
        master_index = i;
        found_master = true;
      }
      clients_on_monitor++;
  }

  if (clients_on_monitor == 0 || !found_master) return;

  Client *master = &wm.client_windows[master_index];

  if (wm.current_layout == WINDOW_LAYOUT_TILED){
    if (clients_on_monitor == 1){
      set_fullscreen(master->frame);
      return;
    }
    int fixed = (1366 / clients_on_monitor) - wm.window_gap;
    int x = wm.window_gap;
    for (uint32_t i = 0; i < clients_on_monitor; i++){
      resize_client(&wm.client_windows[i], (Vec2) {.x = fixed - wm.window_gap, .y = DISPLAY_HEIGHT - 30});
      move_client(&wm.client_windows[i], (Vec2) {.x = x, .y = 15});
      x += fixed + wm.window_gap;
    }
  }
}

void resize_client(Client *client , Vec2 sz) {
  XWindowAttributes attributes;
  XGetWindowAttributes(wm.display, client -> win, &attributes);

  if (sz.x >= DISPLAY_WIDTH && sz.y >= DISPLAY_HEIGHT){
    client -> fullscreen = true;
    XSetWindowBorderWidth(wm.display, client->frame, 0);
    client -> fullscreen_revert_size = (Vec2) {.x = attributes.width, .y = attributes.height};
    // client -> fullscreen_revert_pos = (Vec2) {.x = attributes.x, .y = attributes.y};
  }
  else {
    client -> fullscreen = false;
    XSetWindowBorderWidth(wm.display, client->frame, BORDER_WIDTH);
  }

  XResizeWindow(wm.display, client -> win, sz.x, sz.y);
  XResizeWindow(wm.display, client -> frame, sz.x, sz.y);
  XRaiseWindow(wm.display, client -> frame);
}

void move_client(Client *client, Vec2 pos){
  XMoveWindow(wm.display, client -> frame, pos.x, pos.y);
}


void cycle_window(Window win){
  Client client;
  for (uint32_t i = 0; i < wm.clients_count; i++){
    if (wm.client_windows[i].win == win || wm.client_windows[i].frame == win) {
      if (i + 1 >= wm.clients_count){
        client = wm.client_windows[0];
      }
      else {
        client = wm.client_windows[i + 1];
      }
    }
  }
  XRaiseWindow(wm.display, client.frame);
  XSetInputFocus(wm.display, client.win, RevertToPointerRoot, CurrentTime);
}

void set_fullscreen(Window win){

  if (win == wm.root) return;

  uint32_t client_index = get_client_index(win);
  if (wm.client_windows[client_index].fullscreen) return;

  XWindowAttributes attribs;
  XGetWindowAttributes(wm.display, wm.client_windows[client_index].frame, &attribs);

  wm.client_windows[client_index].fullscreen_revert_pos = (Vec2) {.x = attribs.x, .y = attribs.y};

  resize_client(&wm.client_windows[client_index], (Vec2) {.x = (float) DISPLAY_WIDTH, .y =  768} );

  move_client(&wm.client_windows[client_index], (Vec2){.x = 0, .y = 0});
}

void unset_fullscreen(Window win){
  if (win == wm.root) return;
  const uint32_t client_index = get_client_index(win);

  resize_client(&wm.client_windows[client_index], wm.client_windows[client_index].fullscreen_revert_size);
  move_client(&wm.client_windows[client_index], wm.client_windows[client_index].fullscreen_revert_pos);
}


//others but dorakri start
void window_frame(Window win){
  if (get_client_index(win) != -1) return;
  XWindowAttributes attribs;
  XGetWindowAttributes(wm.display, win, &attribs);

  const Window win_frame = XCreateSimpleWindow(
    wm.display,
    wm.root,
    attribs.x,
    attribs.y,
    attribs.width,
    attribs.height,
    BORDER_WIDTH,
    FBORDER_COLOR,
    BG_COLOR
    );
  XCompositeRedirectWindow(wm.display, win_frame, CompositeRedirectAutomatic);
  //Select Input for the win_frame
  XSelectInput(wm.display, win_frame, SubstructureNotifyMask | SubstructureRedirectMask);
  XAddToSaveSet(wm.display, win_frame);
  XReparentWindow(wm.display, win, win_frame, 0, 0);
  XMapWindow(wm.display, win_frame);
  XSetInputFocus(wm.display, win, RevertToPointerRoot, CurrentTime);

  wm.client_windows[wm.clients_count++] = (Client) {.win = win, .frame = win_frame, .fullscreen = attribs.width >= DISPLAY_WIDTH && attribs.height >= DISPLAY_HEIGHT};
  grab_window_key(win);
  wm.client_windows[wm.clients_count].is_floating = false;
  establish_window_layout(false);
}

void window_unframe(Window win){
  const int32_t client_index = get_client_index(win);

  if (client_index == -1) {printf("Returning from unframe");return;}
  const Window frame_window = wm.client_windows[client_index].frame;

  XReparentWindow(wm.display, frame_window, wm.root, 0, 0);
  XReparentWindow(wm.display, win, wm.root, 0, 0);
  XSetInputFocus(wm.display, wm.root, RevertToPointerRoot, CurrentTime);
  XUnmapWindow(wm.display, frame_window);

  for (uint64_t i = client_index; i < wm.clients_count - 1; i++)
  {
    wm.client_windows[i] = wm.client_windows[i + 1];
  }
  wm.clients_count --;
  establish_window_layout(false);
}
//others but dorakri end


//choto khato functions start

int32_t get_client_index(Window win){
  for (int32_t i  = 0; i < wm.clients_count; i++)
    if (wm.client_windows[i].win == win || wm.client_windows[i].frame == win)
      return i;
  return -1;
}

Window get_frame_window(Window win){
  for (uint32_t i = 0; i < wm.clients_count; i++)
    if (wm.client_windows[i].win  == win || wm.client_windows[i].frame == win)
      return wm.client_windows[i].frame;
  return 0;
}

//choto khato functions end

//DORKARI FUNCTIONS
void handle_map_request(XMapRequestEvent e){
  window_frame(e.window);
  XMapWindow(wm.display, e.window);
}

void handle_unmap_notify(XUnmapEvent e){

  if (get_client_index(e.window) == -1) {
    printf("Ignore UnmapNotify for non-client window\n");
    return;
  }
  printf("Unmapping Window\n");
  window_unframe(e.window);
}

void handle_configure_request(XConfigureRequestEvent e){
  {
  XWindowChanges changes;
  changes.x = e.x;
  changes.y = e.y;
  changes.height = e.height;
  changes.width = e.width;
  changes.border_width = e.border_width;
  changes.sibling = e.above;
  changes.stack_mode = e.detail;
  XConfigureWindow(wm.display, e.window, e.value_mask, &changes);
  }
  {
  XWindowChanges changes;
  changes.x = e.x;
  changes.y = e.y;
  changes.height = e.height;
  changes.width = e.width;
  changes.border_width = e.border_width;
  changes.sibling = e.above;
  changes.stack_mode = e.detail;
  XConfigureWindow(wm.display, get_frame_window(e.window), e.value_mask, &changes);
  }
}

void handle_button_press(XButtonEvent e){
  Window frame = get_frame_window(e.window);
  wm.cursor_start_pos = (Vec2) {.x = (float)e.x_root, .y = (float)e.y_root};
  Window root;
  int32_t x, y;
  unsigned width, height, border_width, depth;

  XGetGeometry(wm.display, frame, &root, &x, &y, &width, &height, &border_width, &depth);
  wm.cursor_start_frame_pos = (Vec2){.x = (float)x, .y = (float)y};
  wm.cursor_start_frame_size = (Vec2){.x = (float)width, .y = (float)height};

  XRaiseWindow(wm.display, wm.client_windows[get_client_index(e.window)].frame);
  XSetInputFocus(wm.display, e.window, RevertToPointerRoot, CurrentTime);
}

void handle_motion_notify(XMotionEvent e) {
  Window frame = get_frame_window(e.window);
  Vec2 drag_pos = (Vec2){.x = (float)e.x_root, .y = (float)e.y_root};
  Vec2 delta_drag = (Vec2){.x = drag_pos.x - wm.cursor_start_pos.x,
                           .y = drag_pos.y - wm.cursor_start_pos.y};
  Client * tmp_client = &wm.client_windows[get_client_index(e.window)];

  if (e.state & Button1Mask) {
    /* Pressed alt + left mouse */
    Vec2 drag_dest =
        (Vec2){.x = (float)(wm.cursor_start_frame_pos.x + delta_drag.x),
               .y = (float)(wm.cursor_start_frame_pos.y + delta_drag.y)};
      if (wm.client_windows[get_client_index(e.window)].fullscreen) {
        tmp_client -> fullscreen = false;
        XSetWindowBorderWidth(wm.display, tmp_client -> frame, BORDER_WIDTH);
      }
    move_client(tmp_client, drag_dest);
    if (!tmp_client -> is_floating){
      tmp_client -> is_floating = true;
      establish_window_layout(false);
    }
    XRaiseWindow(wm.display, tmp_client -> frame);
  } else if (e.state & Button3Mask) {
    /* Pressed alt + right mouse*/
    if (wm.client_windows[get_client_index(e.window)].fullscreen) return;

    Vec2 resize_delta =
        (Vec2){.x = MAX(delta_drag.x, -wm.cursor_start_frame_size.x),
               .y = MAX(delta_drag.y, -wm.cursor_start_frame_size.y)};
    Vec2 resize_dest =
        (Vec2){.x = wm.cursor_start_frame_size.x + resize_delta.x,
               .y = wm.cursor_start_frame_size.y + resize_delta.y};
    
    resize_client(tmp_client, resize_dest);
    if (!tmp_client -> is_floating){
      tmp_client -> is_floating = true;
      establish_window_layout(false);
    }
  }
}


void grab_global_key(){
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, OPEN_TERMINAL), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, OPEN_BROWSER), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, OPEN_LAUNCHER), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, KILL_WM), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, XK_M), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, XK_Up), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, XK_Down), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, XK_Left), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, XK_Right), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, XK_T), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  // XGrabKey(wm.display, XKeysymToKeycode(wm.display, XK_plus), MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
}


void grab_window_key(Window win){
  XGrabButton(wm.display, Button1, MOD, win, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(wm.display, Button3, MOD, win, false, ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, GrabModeAsync, GrabModeAsync, None, None);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, CLOSE_WINDOW), MOD, win, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, CYCLE_WINDOW), MOD, win, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, FULL_SCREEN), MOD, win, false, GrabModeAsync, GrabModeAsync);
  // XGrabKey(wm.display, XKeysymToKeycode(wm.display, TILE_VERT), MOD, win, false, GrabModeAsync, GrabModeAsync);

}

void handle_key_press(XKeyEvent e){
  //if user press MOD Key and Q then it will close the Programm
  if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, CLOSE_WINDOW)) {
      XEvent msg;
      memset(&msg, 0, sizeof(msg));
      msg.xclient.type = ClientMessage;
      msg.xclient.message_type = XInternAtom(wm.display, "WM_PROTOCOLS", false);
      msg.xclient.window = e.window;
      msg.xclient.format = 32;
      msg.xclient.data.l[0] = XInternAtom(wm.display, "WM_DELETE_WINDOW", false);
      XSendEvent(wm.display, e.window, false, 0, &msg);
  }
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, CYCLE_WINDOW)){
    cycle_window(e.window);
  }
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, KILL_WM)) wm.running = false;
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, OPEN_TERMINAL)) system(CMD_TERMINAL);
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, OPEN_BROWSER)) system(CMD_BROWSER);
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, OPEN_LAUNCHER)) system(CMD_APPLAUNCHER);
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, FULL_SCREEN)) {
    if (wm.client_windows[get_client_index(e.window)].fullscreen) {
      unset_fullscreen(e.window);
    }
    else 
      set_fullscreen(e.window);
  }
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, XK_Up)) system(CMD_VOLUME_UP);
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, XK_Down)) system(CMD_VOLUME_DOWN);
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, XK_M)) system(CMD_VOLUME_MUTE);
  else if (e.state & MOD && e.keycode == XKeysymToKeycode(wm.display, XK_T)) establish_window_layout(true);
}


void run_bd26(){

  XSetErrorHandler(handle_wm_detected);
  wm.clients_count = 0;
  wm.cursor_start_frame_size = (Vec2){.x = 0.0f, .y = 0.0f};
  wm.cursor_start_frame_pos = (Vec2){.x = 0.0f, .y = 0.0f};
  wm.cursor_start_pos = (Vec2){.x = 0.0f, .y = 0.0f};
  wm.current_layout = WINDOW_LAYOUT_TILED;
  wm.window_gap = 10;
  wm.running = true;
  XSelectInput(wm.display, wm.root, SubstructureRedirectMask | SubstructureNotifyMask | KeyPressMask | ButtonPressMask);
  XSync(wm.display, false);

  if (wm_detected){
    printf("Another Window Manager is Running\n");
    return;
  }
  XSetErrorHandler(handle_x_error);

  Cursor cursor = XcursorLibraryLoadCursor(wm.display, "arrow");
  XDefineCursor(wm.display, wm.root, cursor);
  XSetErrorHandler(handle_x_error);

  grab_global_key();
  //Dispatch Event Start
  while (wm.running) {
    if (XPending(wm.display)){
    XEvent e;
    XNextEvent(wm.display, &e);
    switch (e.type) {
      //Minor cases
      case CreateNotify:
        handle_create_notify(e.xcreatewindow);break;
      case DestroyNotify:
        handle_destroy_notify(e.xdestroywindow);break;
      case ReparentNotify:
        handle_reparent_notify(e.xreparent);break;
      case ButtonRelease:
        handle_button_release(e.xbutton);break;
      case KeyRelease:
        handle_key_release(e.xkey);break;
      case MapNotify:
        handle_map_notify(e.xmap);break;
      case ConfigureNotify:
        handle_configure_notify(e.xconfigure);break;
      //minor cases end

      case ConfigureRequest:
        handle_configure_request(e.xconfigurerequest);break;
      case MapRequest:
        handle_map_request(e.xmaprequest);break;
      case UnmapNotify:
        handle_unmap_notify(e.xunmap);break;
      case KeyPress:
        handle_key_press(e.xkey);break;
      case ButtonPress:
        handle_button_press(e.xbutton);break;
      case MotionNotify:
        while (XCheckTypedWindowEvent(wm.display, e.xmotion.window, MotionNotify, &e)) {}
        handle_motion_notify(e.xmotion);break;
    }
    }
  }
}

bd26 init_bd26(){
  bd26 tmp;

  tmp.display = XOpenDisplay(NULL);
  if (!tmp.display){
    err(1, "Can not create the connection");
  }
  tmp.root = DefaultRootWindow(tmp.display);
  return tmp;
}

void close_bd26(){
  XCloseDisplay(wm.display);
}

static void run_startup_cmds(){
  uint32_t t = sizeof(startup_commands) / sizeof(startup_commands[0]);
  for (uint32_t i = 0; i < t; i++)
    system(startup_commands[i]);
}

int main(){
  wm = init_bd26();
  run_startup_cmds();
  run_bd26();
  close_bd26();
}

