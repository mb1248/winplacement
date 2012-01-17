/**
 * WinPlacement  
 * Copyright (C) 2010 - Marco Berghoff
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>

#define VERSION "1.2.9"

#define GOLDRATIO 0.618

// TODO fix this values for your desktop setting, or even better use X functions to get this
// information from your desktopmanager
// Border and top are unknown this works by using gravity from the desktopmanager.
#define BORDER 2 
#define TOP 13

// set 1 for debug output
#define DEBUG 1
#define v_printf(...) if (DEBUG) { \
  fprintf(stderr, __VA_ARGS__); \
}

static int findscreen(XineramaScreenInfo *XineramaInfo, int screen_count, XWindowAttributes *window){
  int i;
  for(i=0; i<screen_count; i++){
#if 0 
    //left top corner
    if (   ((window->x >= XineramaInfo[i].x_org) && (window->x < XineramaInfo[i].x_org+XineramaInfo[i].width))
        && ((window->y >= XineramaInfo[i].y_org) && (window->y < XineramaInfo[i].y_org+XineramaInfo[i].height))) {
#else 
    //center
    if (   ((window->x+0.5*window->width >= XineramaInfo[i].x_org) && (window->x+0.5*window->width < XineramaInfo[i].x_org+XineramaInfo[i].width))
        && ((window->y+0.5*window->height >= XineramaInfo[i].y_org) && (window->y+0.5*window->height < XineramaInfo[i].y_org+XineramaInfo[i].height))) {
#endif
      return i;

    }
  }
  return -1;
}

int main (int argc, char **argv) {
  int ret = EXIT_SUCCESS;

  Bool right         = False;
  Bool left          = False;
  Bool top           = False;
  Bool bottom        = False;
  Bool nomax         = False;
  Bool printhelp     = False;
  Bool switchmonitor = False;

  double factor  = 0.5;

  // Options
  static struct option long_options[] = {
    {"help",          no_argument, 0, 'h'},
    {"right",         no_argument, 0, 'r'},
    {"left",          no_argument, 0, 'l'},
    {"top",           no_argument, 0, 't'},
    {"bottom",        no_argument, 0, 'b'},
    {"nomax",         no_argument, 0, 'n'},
    {"goldenratio",   no_argument, 0, 'g'},
    {"Goldenratio",   no_argument, 0, 'G'},
    {"switchmonitor", no_argument, 0, 'm'},
    {0, 0, 0, 0}};

  // getopt_long stores the option index here.
  int option_index = 0;
  int c;
  
  while (1) {
    c = getopt_long (argc, argv, "hrltnbgGm", long_options, &option_index);

    // Detect the end of the options.
    if (c == -1) break;

    switch (c) {
      case 0:
        break;

      case 'h':
        printhelp = True;
        break;

      case 'r':
        v_printf("right\n");
        if (left) {
          fputs("Use left or right.\n", stderr);
          printhelp = True;
        }
        right = True;
        break;

      case 'l':
        v_printf("left\n");
        if (right) {
          fputs("Use left or right.\n", stderr);
          printhelp = True;
        }
        left = True;
        break;

      case 't':
        v_printf("top\n");
        if (bottom) {
          fputs("Use top or bottom.\n", stderr);
          printhelp = True;
        }
        top = True;
        break;

      case 'b':
        v_printf("bottom\n");
        if (top) {
          fputs("Use top or bottom.\n", stderr);
          printhelp = True;
        }
        bottom = True;
        break;

      case 'n':
        v_printf("nomax\n");
        nomax = True;
        break;

      case 'g':
        v_printf("small goldenratio\n");
        if (factor != 0.5) {
          fputs("Use -g or -G.\n", stderr);
          printhelp = True;
        }
        factor = 1.0 - GOLDRATIO;
        break;

      case 'G':
        v_printf("big goldenratio\n");
        if (factor != 0.5) {
          fputs("Use -g or -G.\n", stderr);
          printhelp = True;
        }
        factor = GOLDRATIO;
        break;
      case 'm':
        v_printf("switch monitor\n");
        switchmonitor = True;
        break;
      case '?':
        /* getopt_long already printed an error message. */
        break;

      default:
        abort ();
    }
  }

  // Print help text
  if ((!left  && !right && !top && !bottom && !switchmonitor) || (printhelp)) {
    printf(\
	   "winplacement version "VERSION"\n\n"\
           "Usage: winplacement [options]\n"\
           "  -h, --help,         Print this help.\n\n"\
           "  -r, --right         Move window to the right.\n"\
           "  -l, --left          Move window to the left.\n\n"\
           "  -t, --top           Move window to the top.\n"\
           "  -b, --bottom        Move window to the bottom.\n\n"\
           "  -n, --nomax         Without vertical or horizontal maximizing.\n\n"\
           "  -g, --goldenratio   Use the small part of the golden ratio. (Need -r or -l)\n"\
           "  -G, --Goldenratio   Use the big part of the golden ratio. (Need -r or -l)\n"\
           "  -m, --switchmonitor Change the Monitor.\n"\
          );
    return EXIT_FAILURE;
  }

  // no maximize for edges
  if ( (left || right) && (top || bottom)) {
    nomax = True;
  }

  // Start Xlib stuff
  int actual_format_return;
  unsigned long nitems_return;
  unsigned long bytes_after_return;
  unsigned char *data;

  Display *disp;
  Window win = (Window)0;

  // Check we can get the display
  if (!(disp = XOpenDisplay(NULL))) {
    fputs("Cannot open display.\n", stderr);
    return EXIT_FAILURE;
  }

  // Define Atoms
  Atom ret_atom = (Atom)0;
  Atom maximised_vertically   = XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  Atom maximised_horizontally = XInternAtom(disp, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  Atom normal_window          = XInternAtom(disp, "_NET_WM_WINDOW_TYPE_NORMAL",   False);

  // Get active window
  XGetWindowProperty(disp, DefaultRootWindow(disp), XInternAtom(disp, "_NET_ACTIVE_WINDOW", False),
                     0, 1024, False, XA_WINDOW, &ret_atom,
                     &actual_format_return, &nitems_return, &bytes_after_return, &data);

  win = *((Window *)data);
  XFree(data);

  // Get coordinates
  int x, y;
  Window w_dum = (Window)0;
  XWindowAttributes activ_atr;
  XWindowAttributes root_atr;
  XGetWindowAttributes(disp, win, &activ_atr);
  XTranslateCoordinates (disp, win, activ_atr.root, activ_atr.x, activ_atr.y, &x, &y, &w_dum);
  activ_atr.x=x;
  activ_atr.y=y;
  v_printf("%ux%u @ (%d,%d)/(%d,%d)\n", activ_atr.width, activ_atr.height, x, y, activ_atr.x, activ_atr.y);

  // Get resolution
  XGetWindowAttributes(disp, activ_atr.root, &root_atr);
  v_printf("Resolution: %ux%u\n", root_atr.width, root_atr.height);
  
  // Get workarea
  unsigned long *value;
  XGetWindowProperty(disp, DefaultRootWindow(disp), XInternAtom(disp, "_NET_WORKAREA", False),
		     0, 4*sizeof(unsigned long), False, XA_CARDINAL, &ret_atom,
		     &actual_format_return, &nitems_return, &bytes_after_return, (unsigned char**)&value);

  unsigned long desktop_x = value[0];
  unsigned long desktop_y = value[1];
  unsigned long desktop_w = value[2];
  unsigned long desktop_h = value[3];

  XFree(value);
  v_printf("Workarea: %lux%lu @ (%lu,%lu)\n", desktop_w, desktop_h, desktop_x, desktop_y);
  
  // try to get Xinerama stuff
  
  // Count number of xinerama screens
  XineramaScreenInfo *XineramaInfo;
  int screen_count;
  
  XineramaInfo = XineramaQueryScreens(disp, &screen_count);
  
  v_printf("Screen Number: %d\n", screen_count);
  
  int i;
  for (i=0; i<screen_count; i++){
    v_printf("x,y=(%d,%d) w,h=(%d,%d)\n", XineramaInfo[i].x_org,  XineramaInfo[i].y_org,  XineramaInfo[i].width,  XineramaInfo[i].height);
  }
  
  int activescreen = findscreen(XineramaInfo, screen_count, &activ_atr);
  v_printf("Activescreen: %d\n", activescreen);
  
  desktop_x = XineramaInfo[activescreen].x_org;
  desktop_y = XineramaInfo[activescreen].y_org;
  desktop_w = XineramaInfo[activescreen].width;
  desktop_h = XineramaInfo[activescreen].height;
  
  int newscreen;
  if (switchmonitor) {
    newscreen = (activescreen + 1) % screen_count;
    activ_atr.x = (double)activ_atr.x/XineramaInfo[activescreen].width*XineramaInfo[newscreen].width;
    activ_atr.width = (double)activ_atr.width/XineramaInfo[activescreen].width*XineramaInfo[newscreen].width;
    activ_atr.y = (double)activ_atr.y/XineramaInfo[activescreen].height*XineramaInfo[newscreen].height;
    activ_atr.height = (double)activ_atr.height/XineramaInfo[activescreen].height*XineramaInfo[newscreen].height;
    
    desktop_x = XineramaInfo[newscreen].x_org;
    desktop_y = XineramaInfo[newscreen].y_org;
    desktop_w = XineramaInfo[newscreen].width;
    desktop_h = XineramaInfo[newscreen].height;
  }
  
  Bool isNormalWindow = True;
  
  XGetWindowProperty (disp, win, XInternAtom(disp, "_NET_WM_WINDOW_TYPE", False),
                      0, 1024, False, XA_ATOM, &ret_atom,
                      &actual_format_return, &nitems_return, &bytes_after_return, &data);

  unsigned long *buff2 = (unsigned long *)data;
  for (; nitems_return; nitems_return--) {
    if ( *buff2 != (unsigned long) normal_window) {
      isNormalWindow = False;
      v_printf("Is not normal window\n");
    }
    buff2++;
  }
  XFree(data);

  if (!isNormalWindow) {
    // Exit without change
    XCloseDisplay(disp);
    return ret;
  }

  // Determine maximised state
  Bool isMaxVert = False;
  Bool isMaxHorz = False;
  XGetWindowProperty (disp, win, XInternAtom(disp, "_NET_WM_STATE", False),
                      0, 1024, False, XA_ATOM, &ret_atom,
                      &actual_format_return, &nitems_return, &bytes_after_return, &data);

  unsigned long *buff = (unsigned long *)data;
  for (; nitems_return; nitems_return--) {
    if ( *buff == (unsigned long) maximised_vertically) {
      isMaxVert = True;
      v_printf("Is Vertically maximised\n");
    }
    else if ( *buff == (unsigned long) maximised_horizontally) {
      isMaxHorz = True;
      v_printf("Is Horizontally maximised\n");
    }
    buff++;
  }
  XFree(data);

  // Calculate new Position and set gravity flag
  int new_x = 0, new_y = 0;
  unsigned long grflags = StaticGravity;
  int width = desktop_w * factor - 4 * BORDER + 2;
  int height = (desktop_h) * factor - 2 * BORDER  - 2 * TOP;
  if (switchmonitor) {
    new_x  = activ_atr.x;
    new_y  = activ_atr.y;
//     width  = activ_atr.width;
//     height = activ_atr.height;
  }
  
  if (right) {
    new_x = desktop_x + desktop_w * (1.0 - factor) +  3 * BORDER ;
    v_printf ("Fixing right x\n");
    grflags = EastGravity;
  }
  else if (left) {
    new_x = desktop_x;
    v_printf ("Fixing left x\n");
    grflags = WestGravity;
  }
  
  if (top) {
    new_y = desktop_y;
    v_printf ("Fixing top y\n");
    if (right)
      grflags = NorthEastGravity;
    else if (left)
      grflags = NorthWestGravity;
    else
      grflags = NorthGravity;
  }
  else if (bottom) {
    new_y = desktop_y + desktop_h * (1.0-factor) + 2 * BORDER + 2 * TOP;
    v_printf ("Fixing bottom y\n");
    if (right)
      grflags = SouthEastGravity;
    else if (left)
      grflags = SouthWestGravity;
    else
      grflags = SouthGravity;
  }

  Bool domax_h = False;
  Bool domax_v = False;
  if (top || bottom) {
    if (!nomax) domax_h = True;
    // move y and height
    grflags |= (1 << 9);  //y
    grflags |= (1 << 11); //height
  }

  if (left || right) {
    if (!nomax) domax_v = True;
    // move x and width
    grflags |= (1 << 8);  //x
    grflags |= (1 << 10); //width
  }
  v_printf("FLAG: %lx\n", grflags);

  // remove maximised property; move; set maximised property
  XEvent event;
  long mask = SubstructureRedirectMask | SubstructureNotifyMask;
  event.xclient.type = ClientMessage;
  event.xclient.serial = 0;
  event.xclient.send_event = True;
  event.xclient.window = win;
  event.xclient.format = 32;

  // De-maximise first, seems necessary in some cases
  event.xclient.message_type = XInternAtom(disp, "_NET_WM_STATE", False);
  event.xclient.data.l[0] = (unsigned long)0; // remove/unset property
  event.xclient.data.l[1] = isMaxVert ? (unsigned long)maximised_vertically : 0;
  event.xclient.data.l[2] = isMaxHorz ? (unsigned long)maximised_horizontally : 0;
  XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event);

  // actual move
  event.xclient.message_type = XInternAtom(disp, "_NET_MOVERESIZE_WINDOW", False);
  event.xclient.data.l[0] = grflags; // flags
  event.xclient.data.l[1] = (unsigned long)new_x; //x
  event.xclient.data.l[2] = (unsigned long)new_y; //y
  event.xclient.data.l[3] = (unsigned long)width; //width
  event.xclient.data.l[4] = (unsigned long)height; //height
  XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event);
//   XMoveResizeWindow(disp, win, new_x, new_y, width, height);
//   XSetWindowBorderWidth(disp, win, 10);

  Window root_return;
  int x_return, y_return;
  uint width_return, height_return, border_width_return, depth_return;
  XGetGeometry(disp, win, &root_return, &x_return, &y_return, &width_return, &height_return, &border_width_return, &depth_return);
  v_printf("FLAG: x=%d,y=%d,w=%u,h=%u,b=%u,d=%u\n", x_return, y_return, width_return, height_return, border_width_return, depth_return);

  // restore maximisation state
  event.xclient.message_type = XInternAtom(disp, "_NET_WM_STATE", False);
  event.xclient.data.l[0] = (unsigned long)1; // add/set property
  event.xclient.data.l[1] = (unsigned long)(domax_v) ? (unsigned long)maximised_vertically : 0;
  event.xclient.data.l[2] = (unsigned long)(domax_h) ? (unsigned long)maximised_horizontally : 0;
  XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event);

  XCloseDisplay(disp);

  return ret;
}

