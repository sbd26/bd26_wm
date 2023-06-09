#include <X11/Xcursor/Xcursor.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "struct.h"

int8_t current_workspace = 0;

static bool wm_detected = false;
static bd26 wm;

static void handle_create_notify(XCreateWindowEvent e) { (void)e; }
static void handle_configure_notify(XConfigureEvent e) { (void)e; }
static int handle_wm_detected(Display *display, XErrorEvent *e) {
  (void)display;
  wm_detected = ((int32_t)e->error_code == BadAccess);
  return 0;
}
static void handle_reparent_notify(XReparentEvent e) { (void)e; }
static void handle_destroy_notify(XDestroyWindowEvent e) { (void)e; }
static void handle_map_notify(XMapEvent e) { (void)e; }
static void handle_button_release(XButtonEvent e) { (void)e; }
static void handle_key_release(XKeyEvent e) { (void)e; }
static int handle_x_error(Display *display, XErrorEvent *e) {
  (void)display;
  char err_msg[1024];
  XGetErrorText(display, e->error_code, err_msg, sizeof(err_msg));
  printf("X Error:\n\tRequest: %i\n\tError Code: %i - %s\n\tResource ID: %i\n",
         e->request_code, e->error_code, err_msg, (int)e->resourceid);
  return 0;
}
static void handle_configure_request(XConfigureRequestEvent e);
static void handle_map_request(XMapRequestEvent e); // okay
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
static void move_client(Client *client, Vec2 pos);
static void resize_client(Client *client, Vec2 sz);
static void grab_global_key();
static void grab_window_key(Window win);
static void establish_window_layout(bool restore_back);
static void run_bd26();
static void change_focus_window(Window win);
static void change_active_workspace();
static void print_workspace_number();
static void title_bar_stuff(Client *current_client);
static FontStruct font_create(const char *fontname, const char *fontcolor,
                              Window win);
static void move_another_workspace(Client *client, int32_t workspace);
static void Change_workspace(int32_t t_index);
static int8_t isDialogWindow(Window win);
//------other Functions end

// tiling related function

int8_t isDialogWindow(Window win) {
  Atom windowTypeAtom = XInternAtom(wm.display, "_NET_WM_WINDOW_TYPE", False);
  Atom popupAtom = XInternAtom(wm.display, "_NET_WM_WINDOW_TYPE_DIALOG", False);

  Atom actualType;
  int actualFormat;
  unsigned long itemCount, bytesAfter;
  Atom *atoms;

  if (XGetWindowProperty(wm.display, win, windowTypeAtom, 0, 1024, False,
                         XA_ATOM, &actualType, &actualFormat, &itemCount,
                         &bytesAfter, (unsigned char **)&atoms) == Success) {
    for (unsigned long i = 0; i < itemCount; i++) {
      if (atoms[i] == popupAtom) {
        XFree(atoms);
        return 1; // Pop-up window detected
      }
    }
    XFree(atoms);
  }

  return 0; // Not a pop-up window
}

void move_another_workspace(Client *client, int32_t workspace) {
  if (client->win == wm.root)
    return;

  if (workspace > WORKSPACE - 1) {
    workspace -= WORKSPACE;
  } else if (workspace < 0) {
    workspace += WORKSPACE;
  }
  // save to the target workspace
  wm.client_windows[workspace][wm.clients_count[workspace]++] = *client;
  // remvoe from the current workspace
  uint32_t cli_indx = get_client_index(client->frame);

  Client *tmp_clients[wm.clients_count[current_workspace]];

  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    tmp_clients[i] = &wm.client_windows[current_workspace][i];
  }

  for (uint32_t i = cli_indx; i < wm.clients_count[current_workspace] - 1;
       i++) {
    tmp_clients[i] = tmp_clients[i + 1];
  }

  wm.clients_count[current_workspace]--;

  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    wm.client_windows[current_workspace][i] = *tmp_clients[i];
  }

  XUnmapWindow(wm.display, client->frame);
  wm.swap_done[workspace] = true;
  if (wm.clients_count[current_workspace] != 0) {
    establish_window_layout(False);
    change_focus_window(
        wm.client_windows[current_workspace]
                         [wm.clients_count[current_workspace] - 1]
                             .frame);
  }
}

void draw_str(const char *str, FontStruct font, int x, int y) {
  XftDrawStringUtf8(font.draw, &font.color, font.font, x, y, (XftChar8 *)str,
                    strlen(str));
}

void title_bar_stuff(Client *current_client) {
  XWindowAttributes attribs;
  XGetWindowAttributes(wm.display, current_client->frame, &attribs);
  XGlyphInfo extents;
  // close button
  XClearWindow(wm.display, current_client->decoration.close_button);
  XftTextExtents16(wm.display,
                   current_client->decoration.close_button_font.font,
                   (FcChar16 *)MAXIMIZE_ICON, strlen(MAXIMIZE_ICON), &extents);
  draw_str(MAXIMIZE_ICON, current_client->decoration.close_button_font,
           (ICON_SIZE / 2.0f) - (extents.xOff / 4.0f),
           ((ICON_SIZE - 10) / 2.0f) + (extents.height / 1.25f));

  // maximize icon
  XClearWindow(wm.display, current_client->decoration.maximize_button);
  XftTextExtents16(wm.display,
                   current_client->decoration.maximize_button_font.font,
                   (FcChar16 *)MAXIMIZE_ICON, strlen(MAXIMIZE_ICON), &extents);
  draw_str(MAXIMIZE_ICON, current_client->decoration.maximize_button_font,
           (ICON_SIZE / 2.0f) - (extents.xOff / 4.0f),
           ((ICON_SIZE - 10) / 2.0f) + (extents.height / 1.25f));
  XClearWindow(wm.display, current_client->decoration.title_bar);
  char *window_name = NULL;
  XFetchName(wm.display, current_client->win, &window_name);

  if (window_name != NULL) {
    XGlyphInfo extents;
    XftTextExtents16(wm.display, current_client->decoration.title_bar_font.font,
                     (FcChar16 *)window_name, strlen(window_name), &extents);
    XSetForeground(wm.display, DefaultGC(wm.display, wm.screen),
                   TITLE_BAR_BG_COLOR);
    XFillRectangle(wm.display, current_client->decoration.title_bar,
                   DefaultGC(wm.display, wm.screen), 0, 0, extents.xOff,
                   TITLE_BAR_HEIGHT);
    draw_str(window_name, current_client->decoration.title_bar_font,
             attribs.width / 2 - 50, (TITLE_BAR_HEIGHT / 2 + 5));
    XFree(window_name);
  }
}

FontStruct font_create(const char *fontname, const char *fontcolor,
                       Window win) {
  FontStruct fs;
  XftFont *xft_font = XftFontOpenName(wm.display, 0, fontname);
  XftDraw *xft_draw =
      XftDrawCreate(wm.display, win, DefaultVisual(wm.display, 0),
                    DefaultColormap(wm.display, 0));
  XftColor xft_font_color;
  XftColorAllocName(wm.display, DefaultVisual(wm.display, 0),
                    DefaultColormap(wm.display, 0), fontcolor, &xft_font_color);

  fs.font = xft_font;
  fs.draw = xft_draw;
  fs.color = xft_font_color;
  return fs;
}

void change_active_window() {
  int active_workspace = current_workspace + 1;

  for (int i = 0; i < WORKSPACE; i++) {
    active_workspace %= 4;
    if (wm.clients_count[active_workspace] != 0) {
      active_workspace = active_workspace;
      break;
    }
    active_workspace++;
  }

  if (active_workspace == current_workspace)
    return;

  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    XUnmapWindow(wm.display, wm.client_windows[current_workspace][i].frame);
  }
  for (uint32_t i = 0; i < wm.clients_count[active_workspace]; i++) {
    XMapWindow(wm.display, wm.client_windows[active_workspace][i].frame);
    if (wm.client_windows[active_workspace][i].was_focused)
      XSetInputFocus(wm.display, wm.client_windows[active_workspace][i].win,
                     RevertToPointerRoot, CurrentTime);
  }
  current_workspace = active_workspace;
  print_workspace_number();
}

void change_focus_window(Window win) {
  uint32_t client_index = get_client_index(win);

  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    XSetWindowBorder(wm.display, wm.client_windows[current_workspace][i].frame,
                     UBORDER_COLOR);
    wm.client_windows[current_workspace][i].was_focused = false;
  }
  XSetWindowBorder(wm.display,
                   wm.client_windows[current_workspace][client_index].frame,
                   FBORDER_COLOR);
  XRaiseWindow(wm.display,
               wm.client_windows[current_workspace][client_index].frame);
  XSetInputFocus(wm.display,
                 wm.client_windows[current_workspace][client_index].win,
                 RevertToPointerRoot, CurrentTime);
  wm.client_windows[current_workspace][client_index].was_focused = true;
}

void print_workspace_number() {
  char command[50];
  sprintf(command, "echo %d > ~/.cache/bd26_util.txt", current_workspace);
  system(command);
}

void swap(Client *client1, Client *client2) {
  Client tmp = *client1;
  *client1 = *client2;
  *client2 = tmp;
  establish_window_layout(false);
}

void Change_workspace(int32_t t_index) {
  if (current_workspace == t_index)
    return;
  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    XUnmapWindow(wm.display, wm.client_windows[current_workspace][i].frame);
    XUnmapWindow(
        wm.display,
        wm.client_windows[current_workspace][i].decoration.close_button);
    XUnmapWindow(
        wm.display,
        wm.client_windows[current_workspace][i].decoration.maximize_button);
  }
  current_workspace = t_index;
  if (current_workspace >= WORKSPACE)
    current_workspace -= WORKSPACE;
  else if (current_workspace < 0)
    current_workspace += WORKSPACE;
  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    XMapWindow(wm.display, wm.client_windows[current_workspace][i].frame);
    XMapWindow(
        wm.display,
        wm.client_windows[current_workspace][i].decoration.maximize_button);
    XMapWindow(wm.display,
               wm.client_windows[current_workspace][i].decoration.close_button);
    title_bar_stuff(&wm.client_windows[current_workspace][i]);
    if (wm.client_windows[current_workspace][i].was_focused)
      XSetInputFocus(wm.display, wm.client_windows[current_workspace][i].win,
                     RevertToPointerRoot, CurrentTime);
  }
  if (wm.swap_done[current_workspace]) {
    establish_window_layout(false);
    if (wm.clients_count[current_workspace] > 1)
      change_focus_window(
        wm.client_windows[current_workspace]
                         [wm.clients_count[current_workspace] - 1]
                             .frame);
    wm.swap_done[current_workspace] = false;
  }
  if (wm.already_running[current_workspace]) {
    establish_window_layout(false);
    wm.already_running[current_workspace] = false;
  }
  print_workspace_number();
}
void establish_window_layout(bool restore_back) {
  Client *tmp_clients[CLIENT_WINDOW_CAP];
  uint32_t clients_count = 0;

  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    if (!wm.client_windows[current_workspace][i].is_floating || restore_back) {
      tmp_clients[clients_count++] = &wm.client_windows[current_workspace][i];
    }
  }
  if (restore_back) {
    for (uint32_t i = 0; i < clients_count; i++) {
      tmp_clients[i]->is_floating = false;
    }
  }

  if (clients_count == 0)
    return;
  if (wm.current_layout[current_workspace] == WINDOW_LAYOUT_TILED) {
    Client *rooT = tmp_clients[0];

    if (clients_count == 1) {

      XWindowAttributes attribs;
      XGetWindowAttributes(wm.display, rooT->frame, &attribs);

      wm.client_windows[current_workspace][0].fullscreen_revert_pos =
          (Vec2){.x = attribs.x, .y = attribs.y};
      wm.client_windows[current_workspace][0].fullscreen_revert_size =
          (Vec2){.x = attribs.width, .y = attribs.height};

      resize_client(&wm.client_windows[current_workspace][0],
                    (Vec2){.x = (float)wm.display_width -
                                (float)wm.gaps[current_workspace] * 2,
                           .y = wm.display_height -
                                (float)wm.gaps[current_workspace] * 2});

      move_client(
          &wm.client_windows[current_workspace][0],
          (Vec2){.x = (float)wm.gaps[current_workspace],
                 .y = (float)wm.gaps[current_workspace] + (float)BAR_SIZE});

      if (wm.client_windows[current_workspace][0]
                  .decoration.maximize_button_font.font != NULL &&
          wm.client_windows[current_workspace][0]
                  .decoration.title_bar_font.font != NULL &&
          wm.client_windows[current_workspace][0]
                  .decoration.maximize_button_font.font != NULL)
        title_bar_stuff(&wm.client_windows[current_workspace][0]);
      return;
    }

    resize_client(rooT, (Vec2){.x = (float)wm.display_width / 2 -
                                    (float)wm.gaps[current_workspace],
                               .y = wm.display_height -
                                    (float)wm.gaps[current_workspace] * 2});
    move_client(
        rooT, (Vec2){.x = (float)wm.gaps[current_workspace],
                     .y = (float)BAR_SIZE + (float)wm.gaps[current_workspace]});
    title_bar_stuff(rooT);
    rooT->fullscreen = false;
    float y_cordintae = (float)BAR_SIZE + (float)wm.gaps[current_workspace];

    for (uint32_t i = 1; i < clients_count; i++) {
      resize_client(tmp_clients[i],
                    (Vec2){.x = (float)wm.display_width / 2 -
                                (float)wm.gaps[current_workspace] * 2,
                           .y = (float)wm.display_height / (clients_count - 1) -
                                (float)wm.gaps[current_workspace] * 2});
      move_client(tmp_clients[i], (Vec2){.x = (float)wm.display_width / 2 +
                                              (float)wm.gaps[current_workspace],
                                         .y = y_cordintae});
      y_cordintae += ((float)wm.display_height / (clients_count - 1));
      if (tmp_clients[i]->decoration.title_bar_font.font != NULL &&
          tmp_clients[i]->decoration.close_button_font.font != NULL &&
          tmp_clients[i]->decoration.maximize_button_font.font != NULL)
        title_bar_stuff(tmp_clients[i]);
    }
  } else if (wm.current_layout[current_workspace] ==
             WINDOW_LAYOUT_TILED_VERTICAL) {
    Client *rooT = tmp_clients[0];
    if (clients_count == 1) {
      XWindowAttributes attribs;
      XGetWindowAttributes(wm.display, rooT->frame, &attribs);

      wm.client_windows[current_workspace][0].fullscreen_revert_pos =
          (Vec2){.x = attribs.x, .y = attribs.y};
      wm.client_windows[current_workspace][0].fullscreen_revert_size =
          (Vec2){.x = attribs.width, .y = attribs.height};

      resize_client(&wm.client_windows[current_workspace][0],
                    (Vec2){.x = (float)wm.display_width -
                                (float)wm.gaps[current_workspace] * 2,
                           .y = wm.display_height -
                                (float)wm.gaps[current_workspace] * 2});

      move_client(
          &wm.client_windows[current_workspace][0],
          (Vec2){.x = (float)wm.gaps[current_workspace],
                 .y = (float)wm.gaps[current_workspace] + (float)BAR_SIZE});

      if (wm.client_windows[current_workspace][0]
                  .decoration.maximize_button_font.font != NULL &&
          wm.client_windows[current_workspace][0]
                  .decoration.title_bar_font.font != NULL &&
          wm.client_windows[current_workspace][0]
                  .decoration.maximize_button_font.font != NULL)
        title_bar_stuff(&wm.client_windows[current_workspace][0]);
      return;
    }
    resize_client(rooT, (Vec2){.x = (float)wm.display_width -
                                    wm.gaps[current_workspace] * 2.00,
                               .y = (float)wm.display_height / 2.00 -
                                    (float)wm.gaps[current_workspace] * 2});
    move_client(rooT,
                (Vec2){.x = wm.gaps[current_workspace],
                       .y = (float)BAR_SIZE + wm.gaps[current_workspace]});
    title_bar_stuff(rooT);
    float x_cordinate = wm.gaps[current_workspace];
    for (uint32_t i = 1; i < clients_count; i++) {
      resize_client(tmp_clients[i],
                    (Vec2){.x = (float)wm.display_width / (clients_count - 1) -
                                (float)wm.gaps[current_workspace] * 2,
                           .y = (float)wm.display_height / 2 -
                                (float)wm.gaps[current_workspace] * 2});
      move_client(tmp_clients[i],
                  (Vec2){.x = x_cordinate,
                         .y = (float)wm.display_height / 2 +
                              (float)wm.gaps[current_workspace] * 3});
      x_cordinate += (float)wm.display_width / (clients_count - 1);
    }
  }
}

void resize_client(Client *client, Vec2 sz) {
  XWindowAttributes attributes;
  XGetWindowAttributes(wm.display, client->win, &attributes);

  if (sz.x >= wm.display_width && sz.y >= wm.display_height) {
    client->fullscreen = true;
    XSetWindowBorderWidth(wm.display, client->frame, 0);
    client->fullscreen_revert_size =
        (Vec2){.x = attributes.width, .y = attributes.height};
  } else {
    client->fullscreen = false;
    XSetWindowBorderWidth(wm.display, client->frame, BORDER_WIDTH);
  }

  XMoveWindow(wm.display, client->win, 0, TITLE_BAR_HEIGHT);
  XResizeWindow(wm.display, client->decoration.title_bar, sz.x,
                TITLE_BAR_HEIGHT);
  XResizeWindow(wm.display, client->win, sz.x, sz.y - TITLE_BAR_HEIGHT);
  XResizeWindow(wm.display, client->frame, sz.x, sz.y);
}

void move_client(Client *client, Vec2 pos) {
  XMoveWindow(wm.display, client->frame, pos.x, pos.y);
}

void set_fullscreen(Window win) {
  if (win == wm.root)
    return;

  uint32_t client_index = get_client_index(win);
  if (wm.client_windows[current_workspace][client_index].fullscreen)
    return;

  XWindowAttributes attribs;
  XGetWindowAttributes(wm.display,
                       wm.client_windows[current_workspace][client_index].frame,
                       &attribs);

  wm.client_windows[current_workspace][client_index].fullscreen_revert_pos =
      (Vec2){.x = attribs.x, .y = attribs.y};
  wm.client_windows[current_workspace][client_index].fullscreen_revert_size =
      (Vec2){.x = attribs.width, .y = attribs.height};

  resize_client(&wm.client_windows[current_workspace][client_index],
                (Vec2){.x = (float)wm.display_width,
                       .y = wm.display_height + (float)BAR_SIZE});

  move_client(&wm.client_windows[current_workspace][client_index],
              (Vec2){.x = 0, .y = 0});
  wm.client_windows[current_workspace][client_index].fullscreen = true;

  if (wm.client_windows[current_workspace][client_index]
              .decoration.maximize_button_font.font != NULL &&
      wm.client_windows[current_workspace][client_index]
              .decoration.title_bar_font.font != NULL &&
      wm.client_windows[current_workspace][client_index]
              .decoration.maximize_button_font.font != NULL)
    title_bar_stuff(&wm.client_windows[current_workspace][client_index]);

  change_focus_window(win);
}

void unset_fullscreen(Window win) {
  if (win == wm.root)
    return;
  const uint32_t client_index = get_client_index(win);

  resize_client(&wm.client_windows[current_workspace][client_index],
                (Vec2){wm.client_windows[current_workspace][client_index]
                           .fullscreen_revert_size.x,
                       wm.client_windows[current_workspace][client_index]
                               .fullscreen_revert_size.y +
                           (float)TITLE_BAR_HEIGHT});
  move_client(
      &wm.client_windows[current_workspace][client_index],
      wm.client_windows[current_workspace][client_index].fullscreen_revert_pos);
  wm.client_windows[current_workspace][client_index].fullscreen = false;

  if (wm.client_windows[current_workspace][client_index]
              .decoration.maximize_button_font.font != NULL &&
      wm.client_windows[current_workspace][client_index]
              .decoration.title_bar_font.font != NULL &&
      wm.client_windows[current_workspace][client_index]
              .decoration.maximize_button_font.font != NULL)
    title_bar_stuff(&wm.client_windows[current_workspace][client_index]);
}

// others but dorakri start
void window_frame(Window win) {
  if (get_client_index(win) != -1)
    return;
  XWindowAttributes attribs;
  XGetWindowAttributes(wm.display, win, &attribs);

  const Window win_frame = XCreateSimpleWindow(
      wm.display, wm.root, attribs.x, attribs.y, attribs.width, attribs.height,
      BORDER_WIDTH, FBORDER_COLOR, BG_COLOR);

  XCompositeRedirectWindow(wm.display, win_frame, CompositeRedirectAutomatic);
  // Select Input for the win_frame
  XSelectInput(wm.display, win_frame,
               SubstructureNotifyMask | SubstructureRedirectMask);
  XAddToSaveSet(wm.display, win_frame);
  XReparentWindow(wm.display, win, win_frame, 0, 0);
  XResizeWindow(wm.display, win, attribs.width,
                attribs.height - TITLE_BAR_HEIGHT);
  XMoveWindow(wm.display, win, 0, TITLE_BAR_HEIGHT);

  bool changed = False;
  int8_t tmp_current_workspace = current_workspace;
  int i;
  XClassHint classhint;
  if (XGetClassHint(wm.display, win, &classhint)) {
    for (i = 0; i < sizeof(rules) / sizeof(rules[0]); i++) {
      if (strcmp(classhint.res_class, rules[i].app_name) == 0) {
        current_workspace = rules[i].t_workspace;
        changed = True;
        wm.already_running[current_workspace] = true;
        break;
      }
    }
    XSetStandardProperties(wm.display, win_frame, classhint.res_class, NULL,
                           None, NULL, 0, NULL);
  }
  XFree(classhint.res_class);

  wm.client_windows[current_workspace][wm.clients_count[current_workspace]++] =
      (Client){.win = win,
               .frame = win_frame,
               .fullscreen = attribs.width >= wm.display_width &&
                             attribs.height >= wm.display_height,
               .is_floating = rules[i].is_floating};
  grab_window_key(win);
  wm.client_windows[current_workspace][wm.clients_count[current_workspace]]
      .is_floating = false;

  if (isDialogWindow(win)) {
    wm.client_windows[current_workspace][get_client_index(win)].is_floating =
        true;
  }

  establish_window_layout(false);
  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    wm.client_windows[current_workspace][i].was_focused = false;
  }
  wm.client_windows[current_workspace][get_client_index(win_frame)]
      .was_focused = true;

  int32_t client_index = get_client_index(win);
  Client *current_client = &wm.client_windows[current_workspace][client_index];

  // toy window
  Window helper_window = XCreateSimpleWindow(
      wm.display, wm.root, 50, 200, attribs.width, attribs.height, BORDER_WIDTH,
      TITLE_BAR_BG_COLOR, TITLE_BAR_BG_COLOR);
  XUnmapWindow(wm.display, helper_window);

  XWindowAttributes attribs_frame;
  XGetWindowAttributes(wm.display, win_frame, &attribs_frame);

  // Creating The title Bar
  current_client->decoration.title_bar =
      XCreateSimpleWindow(wm.display, helper_window, 0, 0, attribs_frame.width,
                          TITLE_BAR_HEIGHT, 0, 0, TITLE_BAR_BG_COLOR);
  XSelectInput(wm.display, current_client->decoration.title_bar,
               SubstructureRedirectMask | SubstructureNotifyMask);
  XReparentWindow(wm.display, current_client->decoration.title_bar, win_frame,
                  0, 0);
  XMapWindow(wm.display, current_client->decoration.title_bar);

  // Creating the Close Window
  current_client->decoration.close_button = XCreateSimpleWindow(
      wm.display, helper_window, 0, 0, ICON_SIZE, TITLE_BAR_HEIGHT, 0,
      FBORDER_COLOR, TITLE_BAR_BG_COLOR);
  XSelectInput(wm.display, current_client->decoration.close_button,
               SubstructureNotifyMask | SubstructureRedirectMask |
                   ButtonPressMask);
  XReparentWindow(wm.display, current_client->decoration.close_button,
                  win_frame, 0, 0);
  XMapWindow(wm.display, current_client->decoration.close_button);

  // Creating The Maximize window
  current_client->decoration.maximize_button = XCreateSimpleWindow(
      wm.display, helper_window, 0, 0, ICON_SIZE, TITLE_BAR_HEIGHT, 0,
      FBORDER_COLOR, TITLE_BAR_BG_COLOR);
  XSelectInput(wm.display, current_client->decoration.maximize_button,
               SubstructureNotifyMask | SubstructureRedirectMask |
                   ButtonPressMask);
  XReparentWindow(wm.display, current_client->decoration.maximize_button,
                  win_frame, BUTTON_GAPS, 0);
  XMapWindow(wm.display, current_client->decoration.maximize_button);

  // Font Config
  current_client->decoration.maximize_button_font = font_create(
      FONT, MAXIMIZE_ICON_COLOR, current_client->decoration.maximize_button);
  current_client->decoration.close_button_font = font_create(
      FONT, CLOSE_ICON_COLOR, current_client->decoration.close_button);
  current_client->decoration.title_bar_font =
      font_create(FONT, "#a7a6b4", current_client->decoration.title_bar);

  change_focus_window(win);
  if (current_workspace == tmp_current_workspace || !changed) {
    XMapWindow(wm.display, win_frame);
    XSetInputFocus(wm.display, win, RevertToPointerRoot, CurrentTime);
  }
  current_workspace = tmp_current_workspace;

  if (rules[i].will_focused) {
    Change_workspace(rules[i].t_workspace);
  }
  title_bar_stuff(current_client);
}

void window_unframe(Window win) {
  int32_t client_index = get_client_index(win);

  if (client_index == -1 || win == wm.root) {
    printf("Returning from unframe");
    return;
  }
  const Window frame_window =
      wm.client_windows[current_workspace][client_index].frame;

  XReparentWindow(wm.display, frame_window, wm.root, 0, 0);
  XReparentWindow(wm.display, win, wm.root, 0, 0);
  XUnmapWindow(wm.display, frame_window);

  for (uint32_t i = client_index; i < wm.clients_count[current_workspace] - 1;
       i++) {
    printf("CLIENT INDEX IS %d : %d\n\n\n\n",
           wm.clients_count[current_workspace], i);
    wm.client_windows[current_workspace][i] =
        wm.client_windows[current_workspace][i + 1];
  }
  --wm.clients_count[current_workspace];
  if (wm.clients_count[current_workspace] != 0) {
    if (client_index == 0)
      client_index = 1;
    change_focus_window(
        wm.client_windows[current_workspace][client_index - 1].win);
  }

    establish_window_layout(false);
}
// others but dorakri end

// choto khato functions start

int32_t get_client_index(Window win) {
  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++)
    if (wm.client_windows[current_workspace][i].win == win ||
        wm.client_windows[current_workspace][i].frame == win)
      return i;
  return -1;
}

Window get_frame_window(Window win) {
  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++)
    if (wm.client_windows[current_workspace][i].win == win ||
        wm.client_windows[current_workspace][i].frame == win)
      return wm.client_windows[current_workspace][i].frame;
  return 0;
}

// choto khato functions end

// DORKARI FUNCTIONS
void handle_map_request(XMapRequestEvent e) {
  window_frame(e.window);
  XMapWindow(wm.display, e.window);
  XSetInputFocus(wm.display, e.window, RevertToPointerRoot, CurrentTime);
}

void handle_unmap_notify(XUnmapEvent e) {
  if (get_client_index(e.window) == -1) {
    printf("Ignore UnmapNotify for non-client window\n");
    return;
  }
  window_unframe(e.window);
  // if (wm.clients_count[current_workspace] == 0)
  // wm.gaps[current_workspace] = 0;
}

void handle_configure_request(XConfigureRequestEvent e) {

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
    XConfigureWindow(wm.display, get_frame_window(e.window), e.value_mask,
                     &changes);
  }
}

void handle_button_press(XButtonEvent e) {
  Window frame = get_frame_window(e.window);
  wm.cursor_start_pos = (Vec2){.x = (float)e.x_root, .y = (float)e.y_root};
  Window root;
  int32_t x, y;
  unsigned width, height, border_width, depth;

  XGetGeometry(wm.display, frame, &root, &x, &y, &width, &height, &border_width,
               &depth);
  wm.cursor_start_frame_pos = (Vec2){.x = (float)x, .y = (float)y};
  wm.cursor_start_frame_size = (Vec2){.x = (float)width, .y = (float)height};

  XRaiseWindow(
      wm.display,
      wm.client_windows[current_workspace][get_client_index(e.window)].frame);

  XSetInputFocus(wm.display, e.window, RevertToPointerRoot, CurrentTime);

  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    if (e.window ==
        wm.client_windows[current_workspace][i].decoration.maximize_button) {
      printf("Fullscreen Pressed\n\n\n");
      if (wm.client_windows[current_workspace][i].fullscreen) {
        unset_fullscreen(wm.client_windows[current_workspace][i].frame);
      } else {
        set_fullscreen(wm.client_windows[current_workspace][i].frame);
      }
      break;
    }
  }

  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    if (e.window ==
        wm.client_windows[current_workspace][i].decoration.close_button) {
      XEvent msg;
      memset(&msg, 0, sizeof(msg));
      msg.xclient.type = ClientMessage;
      msg.xclient.message_type = XInternAtom(wm.display, "WM_PROTOCOLS", false);
      msg.xclient.window = wm.client_windows[current_workspace][i].win;
      msg.xclient.format = 32;
      msg.xclient.data.l[0] =
          XInternAtom(wm.display, "WM_DELETE_WINDOW", false);
      XSendEvent(wm.display, wm.client_windows[current_workspace][i].win, false,
                 0, &msg);
      break;
    }
  }
}

void handle_motion_notify(XMotionEvent e) {
  // Window frame = get_frame_window(e.window);
  Vec2 drag_pos = (Vec2){.x = (float)e.x_root, .y = (float)e.y_root};
  Vec2 delta_drag = (Vec2){.x = drag_pos.x - wm.cursor_start_pos.x,
                           .y = drag_pos.y - wm.cursor_start_pos.y};
  Client *tmp_client =
      &wm.client_windows[current_workspace][get_client_index(e.window)];

  if (e.state & Button1Mask) {
    /* Pressed MOD + left mouse */
    Vec2 drag_dest =
        (Vec2){.x = (float)(wm.cursor_start_frame_pos.x + delta_drag.x),
               .y = (float)(wm.cursor_start_frame_pos.y + delta_drag.y)};
    if (wm.client_windows[current_workspace][get_client_index(e.window)]
            .fullscreen) {
      tmp_client->fullscreen = false;
    }
    change_focus_window(e.window);
    move_client(tmp_client, drag_dest);
    if (!tmp_client->is_floating) {
      tmp_client->is_floating = true;
      establish_window_layout(false);
    }
    title_bar_stuff(
        &wm.client_windows[current_workspace][get_client_index(e.window)]);
  } else if (e.state & Button3Mask) {
    /* Pressed MOD + right mouse*/
    if (wm.client_windows[current_workspace][get_client_index(e.window)]
            .fullscreen)
      return;

    Vec2 resize_delta =
        (Vec2){.x = MAX(delta_drag.x, -wm.cursor_start_frame_size.x),
               .y = MAX(delta_drag.y, -wm.cursor_start_frame_size.y)};
    Vec2 resize_dest =
        (Vec2){.x = wm.cursor_start_frame_size.x + resize_delta.x,
               .y = wm.cursor_start_frame_size.y + resize_delta.y};

    for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
      if (wm.client_windows[current_workspace][i].frame == tmp_client->frame) {
        XSetWindowBorder(wm.display, tmp_client->frame, FBORDER_COLOR);
        continue;
      }
      XSetWindowBorder(wm.display,
                       wm.client_windows[current_workspace][i].frame,
                       UBORDER_COLOR);
    }
    resize_client(tmp_client, resize_dest);
    if (!tmp_client->is_floating) {
      tmp_client->is_floating = true;
      establish_window_layout(false);
    }
    title_bar_stuff(
        &wm.client_windows[current_workspace][get_client_index(e.window)]);
  }
}

void grab_global_key() {
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, OPEN_TERMINAL), MOD,
           wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, OPEN_BROWSER), MOD, wm.root,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, OPEN_LAUNCHER), MOD,
           wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, KILL_WM), MOD, wm.root,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, MAKE_TILE), MOD, wm.root,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, CHANGE_WORKSPACE), MOD,
           wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, VOLUME_UP), ControlMask,
           wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, VOLUME_DOWN), ControlMask,
           wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, VOLUME_MUTE), ControlMask,
           wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, CHANGE_WORKSPACE_BACK), MOD,
           wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, CHANGE_ACTIVE_WORKSPACE),
           MOD, wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, SCREENSHOT_KEY), MOD,
           wm.root, false, GrabModeAsync, GrabModeAsync);

  XGrabKey(wm.display,
           XKeysymToKeycode(wm.display, INCREASE_DECREASE_MASTER_SIZE),
           (MOD | ShiftMask), wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display,
           XKeysymToKeycode(wm.display, INCREASE_DECREASE_MASTER_SIZE), MOD,
           wm.root, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, CHANGE_TILE_STYLE), MOD,
           wm.root, false, GrabModeAsync, GrabModeAsync);
}

void grab_window_key(Window win) {
  XGrabButton(wm.display, Button1, MOD, win, false,
              ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(wm.display, Button3, MOD, win, false,
              ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
              GrabModeAsync, GrabModeAsync, None, None);
  XGrabButton(wm.display, Button1, ShiftMask, win, false,
              ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
              None, None); // for mini state
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, CLOSE_WINDOW), MOD, win,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, FULL_SCREEN), MOD, win,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, SWAP_WINDOW), MOD, win,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, SWAP_UP_DOWN), MOD, win,
           false, GrabModeAsync, GrabModeAsync);

  XGrabKey(wm.display, XKeysymToKeycode(wm.display, SWAP_UP_DOWN),
           MOD | ShiftMask, win, false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, NAVIGATE_UP), MOD, win,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, NAVIGATE_DOWN), MOD, win,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, MOVE_WINDOW_NEXT), MOD, win,
           false, GrabModeAsync, GrabModeAsync);
  XGrabKey(wm.display, XKeysymToKeycode(wm.display, MOVE_WINDOW_PREV), MOD, win,
           false, GrabModeAsync, GrabModeAsync);
}
void handle_key_press(XKeyEvent e) {
  // if user press MOD Key and Q then it will close the Programm
  if (e.state & MOD &&
      e.keycode == XKeysymToKeycode(wm.display, CLOSE_WINDOW)) {
    XEvent msg;
    memset(&msg, 0, sizeof(msg));
    msg.xclient.type = ClientMessage;
    msg.xclient.message_type = XInternAtom(wm.display, "WM_PROTOCOLS", false);
    msg.xclient.window = e.window;
    msg.xclient.format = 32;
    msg.xclient.data.l[0] = XInternAtom(wm.display, "WM_DELETE_WINDOW", false);
    XSendEvent(wm.display, e.window, false, 0, &msg);
  } else if (e.state & MOD &&
             e.keycode == XKeysymToKeycode(wm.display, KILL_WM))
    wm.running = false;
  else if (e.state & MOD &&
           e.keycode == XKeysymToKeycode(wm.display, OPEN_TERMINAL))
    system(CMD_TERMINAL);
  else if (e.state & MOD &&
           e.keycode == XKeysymToKeycode(wm.display, OPEN_BROWSER))
    system(CMD_BROWSER);
  else if (e.state & MOD &&
           e.keycode == XKeysymToKeycode(wm.display, OPEN_LAUNCHER))
    system(CMD_APPLAUNCHER);
  else if (e.state & MOD &&
           e.keycode == XKeysymToKeycode(wm.display, FULL_SCREEN)) {
    if (wm.client_windows[current_workspace][get_client_index(e.window)]
            .fullscreen) {
      unset_fullscreen(e.window);
    } else
      set_fullscreen(e.window);
  } else if (e.state & ControlMask &&
             e.keycode == XKeysymToKeycode(wm.display, VOLUME_UP))
    system(CMD_VOLUME_UP);
  else if (e.state & ControlMask &&
           e.keycode == XKeysymToKeycode(wm.display, VOLUME_DOWN))
    system(CMD_VOLUME_DOWN);
  else if (e.state & ControlMask &&
           e.keycode == XKeysymToKeycode(wm.display, VOLUME_MUTE))
    system(CMD_VOLUME_MUTE);
  else if (e.state & MOD &&
           e.keycode == XKeysymToKeycode(wm.display, MAKE_TILE)) {
    establish_window_layout(true);
  } else if (e.state & MOD &&
             e.keycode == XKeysymToKeycode(wm.display, CHANGE_WORKSPACE))
    Change_workspace(current_workspace + 1);
  else if (e.state & MOD &&
           e.keycode == XKeysymToKeycode(wm.display, CHANGE_WORKSPACE_BACK))
    Change_workspace(current_workspace - 1);
  else if (e.state & MOD &&
           e.keycode == XKeysymToKeycode(wm.display, SWAP_WINDOW)) {
    if (wm.clients_count[current_workspace] >= 2) {
      uint32_t client_index = get_client_index(e.window);
      if (client_index == 0) {
        swap(&wm.client_windows[current_workspace][0],
             &wm.client_windows[current_workspace][1]);
      }

      else {
        swap(&wm.client_windows[current_workspace][0],
             &wm.client_windows[current_workspace][client_index]);
      }
    }
  } else if ((e.state & MOD) && (e.state & ShiftMask) &&
             e.keycode == XKeysymToKeycode(wm.display, SWAP_UP_DOWN)) {
    if (wm.clients_count[current_workspace] >= 3) {
      uint32_t tmp_index = get_client_index(e.window);
      if (tmp_index == 0 ||
          tmp_index == wm.clients_count[current_workspace] - 1)
        return;
      swap(&wm.client_windows[current_workspace][tmp_index],
           &wm.client_windows[current_workspace][tmp_index + 1]);
    }

  }

  else if (e.state & MOD &&

           e.keycode == XKeysymToKeycode(wm.display, SWAP_UP_DOWN)) {
    if (wm.clients_count[current_workspace] >= 3) {
      uint32_t tmp_index = get_client_index(e.window);
      if (tmp_index == 0 || tmp_index == 1)
        return;
      swap(&wm.client_windows[current_workspace][tmp_index],
           &wm.client_windows[current_workspace][tmp_index - 1]);
    }
  } else if (e.state & MOD &&
             e.keycode == XKeysymToKeycode(wm.display, NAVIGATE_DOWN)) {
    if (wm.clients_count[current_workspace] > 1) {
      uint32_t client_index = get_client_index(e.window);
      if (client_index == wm.clients_count[current_workspace] - 1)
        client_index = -1;
      change_focus_window(
          wm.client_windows[current_workspace][client_index + 1].win);
    }
  } else if (e.state & MOD &&
             e.keycode == XKeysymToKeycode(wm.display, NAVIGATE_UP)) {
    if (wm.clients_count[current_workspace] > 1) {
      uint32_t client_index = get_client_index(e.window);
      if (client_index == 0)
        client_index = wm.clients_count[current_workspace];
      change_focus_window(
          wm.client_windows[current_workspace][client_index - 1].win);
    }
  } else if (e.state & MOD &&
             e.keycode ==
                 XKeysymToKeycode(wm.display, CHANGE_ACTIVE_WORKSPACE)) {
    change_active_window();
  } else if (e.state & MOD &&
             e.keycode == XKeysymToKeycode(wm.display, MOVE_WINDOW_PREV)) {
    move_another_workspace(
        &wm.client_windows[current_workspace][get_client_index(e.window)],
        current_workspace - 1);
  } else if (e.state & MOD &&
             e.keycode == XKeysymToKeycode(wm.display, MOVE_WINDOW_NEXT)) {
    move_another_workspace(
        &wm.client_windows[current_workspace][get_client_index(e.window)],
        current_workspace + 1);
  } else if (e.state & MOD &&
             e.keycode == XKeysymToKeycode(wm.display, SCREENSHOT_KEY)) {
    system(CMD_SCREENSHOT);
  } else if ((e.state & MOD) && (e.state & ShiftMask) &&
             e.keycode ==
                 XKeysymToKeycode(wm.display, INCREASE_DECREASE_MASTER_SIZE)) {
    wm.gaps[current_workspace] -= 10;
    establish_window_layout(false);
  }

  else if (e.state & MOD &&
           e.keycode ==
               XKeysymToKeycode(wm.display, INCREASE_DECREASE_MASTER_SIZE)) {
    wm.gaps[current_workspace] += 10;
    establish_window_layout(false);
  } else if (e.state & MOD &&
             e.keycode == XKeysymToKeycode(wm.display, CHANGE_TILE_STYLE)) {
    if (wm.current_layout[current_workspace] == 0)
      wm.current_layout[current_workspace] = 1;
    else
      wm.current_layout[current_workspace] = 0;
    establish_window_layout(false);
  }
}

void tt_stuff() {
  for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
    title_bar_stuff(&wm.client_windows[current_workspace][i]);
  }
}

void run_bd26() {

  XSetErrorHandler(handle_wm_detected);
  if (!wm.clients_count[current_workspace])
    wm.clients_count[current_workspace] = 0;

  for (uint32_t i = 0; i < WORKSPACE; i++) {
    wm.clients_count[i] = 0;
    wm.gaps[i] = 10.0;
  }

  wm.screen = DefaultScreen(wm.display);
  wm.display_height = DisplayHeight(wm.display, wm.screen);
  wm.display_height -= (float)BAR_SIZE;
  wm.display_width = DisplayWidth(wm.display, wm.screen);
  wm.cursor_start_frame_size = (Vec2){.x = 0.0f, .y = 0.0f};
  wm.cursor_start_frame_pos = (Vec2){.x = 0.0f, .y = 0.0f};
  wm.cursor_start_pos = (Vec2){.x = 0.0f, .y = 0.0f};
  wm.current_layout[current_workspace] = WINDOW_LAYOUT_TILED;
  wm.running = true;
  XSelectInput(wm.display, wm.root,
               SubstructureRedirectMask | SubstructureNotifyMask |
                   KeyPressMask | ButtonPressMask);
  XSync(wm.display, false);

  if (wm_detected) {
    printf("Another Window Manager is Running\n");
    return;
  }
  XSetErrorHandler(handle_x_error);

  Cursor cursor = XcursorLibraryLoadCursor(wm.display, "arrow");
  XDefineCursor(wm.display, wm.root, cursor);
  XSetErrorHandler(handle_x_error);
  grab_global_key(); // Dispatch Event Start
  while (wm.running) {

    for (uint32_t i = 0; i < wm.clients_count[current_workspace]; i++) {
      title_bar_stuff(&wm.client_windows[current_workspace][i]);
    }

    XEvent e;
    XNextEvent(wm.display, &e);
    switch (e.type) {
    // Minor cases
    case CreateNotify:
      handle_create_notify(e.xcreatewindow);
      break;
    case DestroyNotify:
      handle_destroy_notify(e.xdestroywindow);
      break;
    case ReparentNotify:
      handle_reparent_notify(e.xreparent);
      break;
    case ButtonRelease:
      handle_button_release(e.xbutton);
      break;
    case KeyRelease:
      handle_key_release(e.xkey);
      break;
    case MapNotify:
      handle_map_notify(e.xmap);
      break;
    case ConfigureNotify:
      handle_configure_notify(e.xconfigure);
      break;
      // minor cases end
      //

    case ConfigureRequest:
      handle_configure_request(e.xconfigurerequest);
      break;
    case MapRequest:
      handle_map_request(e.xmaprequest);
      break;
    case UnmapNotify:
      handle_unmap_notify(e.xunmap);
      break;
    case KeyPress:
      handle_key_press(e.xkey);
      break;
    case ButtonPress:
      handle_button_press(e.xbutton);
      break;
    case MotionNotify:
      while (XCheckTypedWindowEvent(wm.display, e.xmotion.window, MotionNotify,
                                    &e)) {
      }
      handle_motion_notify(e.xmotion);
      break;
    }
  }
}

static void run_startup_cmds() {
  uint32_t t = sizeof(startup_commands) / sizeof(startup_commands[0]);
  for (uint32_t i = 0; i < t; i++)
    system(startup_commands[i]);
}

bd26 init_bd26() {
  bd26 tmp;
  run_startup_cmds();
  tmp.display = XOpenDisplay(NULL);
  if (!tmp.display) {
    err(1, "Can not create the connection");
  }
  tmp.root = DefaultRootWindow(tmp.display);
  print_workspace_number();
  return tmp;
}

void close_bd26() { XCloseDisplay(wm.display); }

int main() {
  wm = init_bd26();
  run_bd26();
  close_bd26();
}
